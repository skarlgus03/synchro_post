#include "Grid/GridObstacle.h"
#include "Grid/ObstacleDataAsset.h"
#include "Framework/GridManager.h"
#include "Components/StaticMeshComponent.h"
#include "Types/SPGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "SynchroPost.h"
#include "Unit/UnitHealthBarComponent.h"

AGridObstacle::AGridObstacle()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(Root);

	// 메시에 붙인다 — 높이가 메시 기준이고 배치 스케일을 따라간다 (유닛과 같은 방식)
	HealthBarWidgetComponent = CreateDefaultSubobject<UUnitHealthBarComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(MeshComponent);
}

void AGridObstacle::BeginPlay()
{
	Super::BeginPlay();
	
	RefreshHealthBar();
}

void AGridObstacle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGridObstacle, CurrentHealth);
	DOREPLIFETIME(AGridObstacle, SpawnInfo);
	DOREPLIFETIME(AGridObstacle, bIsDestroyed);
}

void AGridObstacle::InitializeObstacle(const FObstacleSpawnInfo& Info)
{
	if (!Info.ObstacleData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ObstacleData is NULL in InitializeObstacle!"), *GetName());
		return;
	}

	SpawnInfo = Info;
	if (HasAuthority())
	{
		CurrentHealth = Info.ObstacleData->MaxHealth;
		Resistances = Info.ObstacleData->Resistances;
	}
	
	ApplyVisuals();
}

int32 AGridObstacle::ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData)
{
	if (!HasAuthority() || bIsDestroyed)
	{
		return 0;
	}

	int32 FinalAmount = ActionData.Amount;

	// 힐 태그가 없으면 저항 적용 (방어력 계산 없음 - 장애물은 방어력 개념 자체가 없음)
	if (!ActionData.ActionTypeTags.HasTag(SPTags::Action::Heal))
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
			GridManager->ClearOccupantAt(GetGridPosition());
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
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->NotifyHealthPresented(DisplayAmount, NewTargetHealth, bIsCritical, TypeTags);
	}

	if (NewTargetHealth <= 0)
	{
		// 기본 파괴 연출: 액터째 숨김 + 충돌 제거. 메시만 숨기면 피격 유지 타이머가 끝날 때
		// 체력바가 WhenDamaged 규칙으로 다시 켜진다. 특수 연출은 서브클래스에서 오버라이드
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}

void AGridObstacle::SetHovered(bool bHovered)
{
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetHovered(bHovered);
	}
}

void AGridObstacle::SetSelected(bool bSelected)
{
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetSelected(bSelected);
	}
}

void AGridObstacle::OnRep_SpawnInfo()
{
	ApplyVisuals();
}


void AGridObstacle::ApplyVisuals()
{
	const UObstacleDataAsset* Data = SpawnInfo.ObstacleData;
	if (!Data)
	{
		UE_LOG(LogSP, Error, TEXT("[%s] ApplyVisuals : ObstacleData is NULL!"), *GetName());
		return;
	}

	MaxHealth = Data->MaxHealth;

	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(Data->ObstacleMesh.LoadSynchronous());

		const FTransform Placement(SpawnInfo.Rotation, FVector::ZeroVector, SpawnInfo.Scale);
		MeshComponent->SetRelativeTransform(Data->MeshRelativeTransform * Placement);
	}

	RefreshHealthBar();
}

void AGridObstacle::RefreshHealthBar()
{
	const UObstacleDataAsset* Data = SpawnInfo.ObstacleData;
	if (!Data || !HealthBarWidgetComponent)
	{
		return;
	}

	HealthBarWidgetComponent->Refresh(Data->HealthBar, MeshComponent, CurrentHealth, MaxHealth);
}