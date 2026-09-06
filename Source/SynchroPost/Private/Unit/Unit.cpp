#include "Unit/Unit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Unit/UnitDataAsset.h"
#include "Unit/SkillComponent.h"
#include "Unit/StatComponent.h"
#include "Unit/StateComponent.h"
#include "Unit/GridMoveComponent.h"
#include "Framework/CombatEventComponent.h"
#include "Framework/SPGameState.h"
#include "Framework/TurnManager.h"
#include "Framework/GridManager.h"
#include "Types/SPCombatEventStructure.h"
#include "Unit/UnitPresentationBase.h"
#include "Net/UnrealNetwork.h"
#include "Framework/SPGameState.h"
#include "Framework/TurnStateComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/UnitHealthBarWidget.h"
#include "Framework/SynchroPostSettings.h"



// Sets default values
AUnit::AUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	if (UCharacterMovementComponent* CharacterMoveComp = GetCharacterMovement())
	{
		CharacterMoveComp->GravityScale = 0.0f;
		CharacterMoveComp->bComponentShouldUpdatePhysicsVolume = false;
	}

	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	StateComponent = CreateDefaultSubobject<UStateComponent>(TEXT("StateComponent"));
	GridMoveComponent = CreateDefaultSubobject<UGridMoveComponent>(TEXT("GridMoveComponent"));

	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(GetMesh());
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	
	Super::BeginPlay();

	InitializeUnit(nullptr);

	if (!StatComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] StatComponent is NULL in BeginPlay!"), *GetName());
		return;
	}

	StatComponent->OnHealthChanged.AddDynamic(this, &AUnit::HandleHealthChanged);

	if (ASPGameState* SPGameState = GetWorld()->GetGameState<ASPGameState>())
	{
		if (UTurnStateComponent* TurnState = SPGameState->GetTurnStateComponent())
		{
			TurnState->OnUnitTurnStart.AddDynamic(this, &AUnit::HandleTurnStart);
			TurnState->OnUnitTurnEnd.AddDynamic(this, &AUnit::HandleTurnEnd);
		}
	}
}


// Called every frame
void AUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUnit, GridPosition);
	DOREPLIFETIME(AUnit, ReplicatedUnitData);
	DOREPLIFETIME(AUnit, Faction);
}

void AUnit::InitializeUnit(const UUnitDataAsset* UnitData)
{
	if (UnitData)
	{
		CurrentUnitData = UnitData;
	}
	else
	{
		CurrentUnitData = DefaultUnitData;
	}

	if (!CurrentUnitData)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] CurrentUnitData is NULL in InitializeUnit!"), *GetName());
		return;
	}


	if (HasAuthority())
	{
		ReplicatedUnitData = TSoftObjectPtr<UUnitDataAsset>(FSoftObjectPath(CurrentUnitData.Get()));
	}

	if (!CurrentUnitData->UnitMesh.IsNull())
	{
		GetMesh()->SetSkeletalMesh(CurrentUnitData->UnitMesh.LoadSynchronous());
	}
	
	if (SkillComponent)
	{
		SkillComponent->InitializeSkillComponent(CurrentUnitData);
	}
	if (StatComponent)
	{
		StatComponent->InitializeStats(CurrentUnitData->UnitStatData, 1);
		StatComponent->InitializeGimmickStats(CurrentUnitData);
	}

	TSubclassOf<UUnitPresentationBase> PresentationClassToUse = CurrentUnitData->PresentationClass;
	if (!PresentationClassToUse)
	{
		if (const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>())
		{
			PresentationClassToUse = Settings->DefaultPresentationClass;
		}
	}

	if (!PresentationClassToUse)
	{
		PresentationClassToUse = UUnitPresentationBase::StaticClass();
	}
	PresentationBehavior = NewObject<UUnitPresentationBase>(this, PresentationClassToUse);

	if (HealthBarWidgetComponent)
	{
		TSubclassOf<UUnitHealthBarWidget> WidgetClassToUse = CurrentUnitData->HealthBarWidgetClass;
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
			const int32 InitialHealth = StatComponent ? StatComponent->GetCurrentHealth() : 0;
			const int32 InitialMaxHealth = StatComponent ? StatComponent->GetStat(SPTags::Stat::Combat::Primary::MaxHealth) : 0;
			HealthBarWidget->InitializeHealthBar(InitialHealth, InitialMaxHealth);
		}
	}
}

void AUnit::HandleHealthChanged(int32 NewHealth, const FSPHealthActionData& ActionData)
{

	const bool bWasDead = bIsDead;
	bIsDead = (NewHealth <= 0);

	// 상태가 죽음으로 바뀌었거나 회복된 경우
	if (bIsDead && !bWasDead)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] has died."), *GetName());
		OnUnitDied.Broadcast(this);

		if (UCombatEventComponent* EventComp = GetCombatEventComponent())
		{
			FCombatEvent Event;
			Event.Source = this;
			FUnitDiedPayload DiedPayload;
			DiedPayload.Causer = Cast<AUnit>(ActionData.Causer);
			DiedPayload.DeathCoordinate = GetGridPosition();
			Event.Payload = FInstancedStruct::Make(DiedPayload);

			EventComp->PushEvent(Event);
		}
	}
	else if (!bIsDead && bWasDead)
	{
		// 유닛 부활 이벤트 발생
		OnUnitRevived.Broadcast(this);

		if (UCombatEventComponent* EventComp = GetCombatEventComponent())
		{
			FCombatEvent Event;
			Event.Source = this;

			FUnitRevivedPayload RevivedPayload;
			RevivedPayload.Causer = Cast<AUnit>(ActionData.Causer);
			
			// 만약 살릴위치가 다른곳이라면 이쪽 코드 수정해줘야한다.
			// 일단 그냥 죽은 유닛 위치를 넣었다.
			RevivedPayload.RevivalCoordinate = GetGridPosition();
			Event.Payload = FInstancedStruct::Make(RevivedPayload);

			EventComp->PushEvent(Event);
		}
	}
}

void AUnit::HandleTurnStart(AUnit* Unit)
{
	if (Unit != this)
	{
		return;
	}

	if (GridMoveComponent)
	{
		GridMoveComponent->RefillMovePoint();
	}
	if (SkillComponent)
	{
		SkillComponent->HandleUnitTurnStart(this);
	}
	if (StateComponent)
	{
		StateComponent->HandleUnitTurnStart(this);
	}
}

void AUnit::HandleTurnEnd(AUnit* Unit)
{
	if (Unit != this)
	{
		return;
	}

	if (SkillComponent)
	{
		SkillComponent->HandleUnitTurnEnd(this);
	}

	if (StateComponent)
	{
		StateComponent->HandleUnitTurnEnd(this);
	}
}

int32 AUnit::ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData)
{
	if (!StatComponent)
	{
		return 0;
	}
	if (bIsDead)
	{
		return 0;
	}

	return StatComponent->ApplyHealthChange(ActionData);
}

void AUnit::ApplyVisualDamage_Implementation(int32 DisplayAmount, int32 NewTargetHealth, bool bIsCritical, const FGameplayTagContainer& TypeTags)
{
	if (UUnitHealthBarWidget* HealthBarWidget = GetHealthBarWidget())
	{
		HealthBarWidget->AnimateToHealth(NewTargetHealth);
		HealthBarWidget->ShowDamageNumber(DisplayAmount, bIsCritical, TypeTags);
	}
}

int32 AUnit::GetCurrentHealth_Implementation() const
{
	if (StatComponent)
	{
		return StatComponent->GetCurrentHealth();
	}
	return 0;
}


void AUnit::ServerRequestMove_Implementation(const FIntPoint& Destination)
{
	if (GridMoveComponent)
	{
		GridMoveComponent->RequestMove(Destination);
	}
}

void AUnit::ServerExecuteSkill_Implementation(const FGameplayTag& SkillSlotTag, const FSkillTargetData& Target)
{
	if (SkillComponent)
	{
		SkillComponent->ExecuteSkill(SkillSlotTag, Target);
	}
}


void AUnit::OnRep_GridPosition(FIntPoint OldGridPosition)
{

}

void AUnit::OnRep_UnitData()
{
	if (!ReplicatedUnitData.IsNull())
	{
		InitializeUnit(ReplicatedUnitData.LoadSynchronous());
	}
}

void AUnit::OnRep_Faction()
{
	OnFactionChanged.Broadcast(this);
}

void AUnit::PresentDeath()
{
	if (PresentationBehavior)
	{
		PresentationBehavior->PresentDeath(this);
	}
	else
	{
		NotifyMyPresentationFinished();
	}
}

void AUnit::PresentRevive()
{
	if (PresentationBehavior)
	{
		PresentationBehavior->PresentRevive(this);
	}
	else
	{
		NotifyMyPresentationFinished();
	}
}

void AUnit::PresentMoveSegment(const FIntPoint& From, const FIntPoint& To)
{
	if (PresentationBehavior)
	{
		PresentationBehavior->PresentMoveSegment(this, From, To);
	}
	else
	{
		NotifyMyPresentationFinished();
	}
}

void AUnit::NotifyMyPresentationFinished()
{
	if (UCombatEventComponent* EventComp = GetCombatEventComponent())
	{
		EventComp->NotifyPresentationFinished();
	}
}

void AUnit::SetFaction(EFaction NewFaction)
{
	if (Faction != NewFaction)
	{
		Faction = NewFaction;
		OnFactionChanged.Broadcast(this);
	}
}

int32 AUnit::GetSpeed() const
{
	if (StatComponent)
	{
		return StatComponent->GetStat(SPTags::Stat::Combat::Primary::Speed);
	}
	return 0;
}

UCombatEventComponent* AUnit::GetCombatEventComponent() const
{
	if (ASPGameState* GameState = GetWorld()->GetGameState<ASPGameState>())
	{
		return GameState->GetCombatEventComponent();
	}

	return nullptr;
}

UUnitHealthBarWidget* AUnit::GetHealthBarWidget() const
{
	return HealthBarWidgetComponent ? Cast<UUnitHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()) : nullptr;
}
