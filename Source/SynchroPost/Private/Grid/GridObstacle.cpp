#include "Grid/GridObstacle.h"
#include "Grid/ObstacleDataAsset.h"
#include "Framework/GridManager.h"
#include "Components/StaticMeshComponent.h"
#include "Types/SPGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "SynchroPost.h"
#include "Math/DamageMath.h"
#include "Math/StatMath.h"
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

FSPHealthActionData  AGridObstacle::ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData)
{
	if (!HasAuthority() || bIsDestroyed)
	{
		return FSPHealthActionData();
	}

	FSPHealthActionData Result = ActionData;
	const bool bIsHeal = Result.ActionTypeTags.HasTag(SPTags::Action::Heal);

	// 피해면 조각마다 저항 적용 (장애물은 방어력 개념 없음)
	if (bIsHeal)
	{
		Result.SyncTotalsFromParts();
	}
	else
	{
		// 장애물은 방어력 없음(1배). 저항은 DA 맵 — 약점(음수)도 허용
		DamageMath::ResolveDamage(Result,
			[](const FGameplayTagContainer&, const FPenetrationData&) { return 1.f; },
			[this](const FGameplayTagContainer& PartTags)
			{
				float Multiplier = 1.f;
				for (const TPair<FGameplayTag, int32>& Pair : Resistances)
				{
					if (PartTags.HasTag(Pair.Key))
					{
						Multiplier *= StatMath::ResistanceToMultiplier(Pair.Value);
					}
				}
				return Multiplier;
			});
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + (bIsHeal ? Result.Amount : -Result.Amount), 0, MaxHealth);

	if (CurrentHealth <= 0 && !bIsDestroyed)
	{
		bIsDestroyed = true;
		if (UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>())
		{
			GridManager->ClearOccupantAt(GetGridPosition());
		}
	}

	return Result;
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