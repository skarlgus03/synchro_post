#include "Grid/GridObstacle.h"
#include "Grid/ObstacleDataAsset.h"
#include "Framework/GridManager.h"
#include "Framework/SynchroPostSettings.h"
#include "UI/UnitHealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Types/SPGameplayTags.h"
#include "Net/UnrealNetwork.h"

AGridObstacle::AGridObstacle()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(RootComponent);
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
}

void AGridObstacle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGridObstacle, CurrentHealth);
	DOREPLIFETIME(AGridObstacle, GridPosition);
	DOREPLIFETIME(AGridObstacle, bIsDestroyed);
}

void AGridObstacle::InitializeObstacle(const UObstacleDataAsset* ObstacleData)
{
	if (!ObstacleData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ObstacleData is NULL in InitializeObstacle!"), *GetName());
		return;
	}

	CurrentObstacleData = ObstacleData;

	if (HasAuthority())
	{
		MaxHealth = ObstacleData->MaxHealth;
		CurrentHealth = MaxHealth;
		Resistances = ObstacleData->Resistances;
	}

	if (!ObstacleData->ObstacleMesh.IsNull() && MeshComponent)
	{
		MeshComponent->SetStaticMesh(ObstacleData->ObstacleMesh.LoadSynchronous());
	}

	if (HealthBarWidgetComponent)
	{
		TSubclassOf<UUnitHealthBarWidget> WidgetClassToUse = ObstacleData->HealthBarWidgetClass;
		if (!WidgetClassToUse)
		{
			if (const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>())
			{
				WidgetClassToUse = Settings->DefaultHealthBarWidgetClass;
			}
		}
		if (!WidgetClassToUse)
		{
			WidgetClassToUse = UUnitHealthBarWidget::StaticClass();
		}
		HealthBarWidgetComponent->SetWidgetClass(WidgetClassToUse);

		if (UUnitHealthBarWidget* HealthBarWidget = GetHealthBarWidget())
		{
			HealthBarWidget->InitializeHealthBar(CurrentHealth, MaxHealth);
		}
	}
}

int32 AGridObstacle::ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData)
{
	if (!HasAuthority() || bIsDestroyed)
	{
		return 0;
	}

	int32 FinalAmount = ActionData.Amount;

	// 힐 태그가 없으면 저항 적용 (방어력 계산 없음 - 장애물은 방어력 개념 자체가 없음)
	if (!ActionData.ActionTypeTags.HasTag(SPTags::Heal))
	{
		for (const TPair<FGameplayTag, int32>& Pair : Resistances)
		{
			if (ActionData.ActionTypeTags.HasTag(Pair.Key))
			{
				const float ReductionPercent = FMath::Clamp(Pair.Value, 0, 100) / 100.0f;
				FinalAmount = FMath::RoundToInt(FinalAmount * (1.0f - ReductionPercent));
			}
		}
	}

	const int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - FinalAmount, 0, MaxHealth);
	const int32 ActualChange = OldHealth - CurrentHealth;

	if (CurrentHealth <= 0 && !bIsDestroyed)
	{
		bIsDestroyed = true;
		if (UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>())
		{
			GridManager->ClearOccupantAt(GridPosition);
		}
	}

	return ActualChange;
}

int32 AGridObstacle::GetCurrentHealth_Implementation() const
{
	return CurrentHealth;
}

void AGridObstacle::ApplyVisualDamage_Implementation(int32 DisplayAmount, int32 NewTargetHealth, bool bIsCritical, const FGameplayTagContainer& TypeTags)
{
	if (UUnitHealthBarWidget* HealthBarWidget = GetHealthBarWidget())
	{
		HealthBarWidget->AnimateToHealth(NewTargetHealth);
		HealthBarWidget->ShowDamageNumber(DisplayAmount, bIsCritical, TypeTags);
	}

	if (NewTargetHealth <= 0 && MeshComponent)
	{
		// 기본 파괴 연출: 메시 숨김 + 충돌 제거. 특수 연출은 서브클래스에서 이 함수를 오버라이드
		MeshComponent->SetVisibility(false, true);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

UUnitHealthBarWidget* AGridObstacle::GetHealthBarWidget() const
{
	return HealthBarWidgetComponent ? Cast<UUnitHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()) : nullptr;
}