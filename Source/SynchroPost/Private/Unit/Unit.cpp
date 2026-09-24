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
#include "Framework/TurnStateComponent.h"
#include "Framework/SynchroPostSettings.h"
#include "Unit/UnitAnimSetDataAsset.h"
#include "Unit/UnitHealthBarComponent.h"
#include "Components/CapsuleComponent.h"
#include "Slot/UnitSlotComponent.h"
#include "Unit/UnitAttachment.h"
#include "SynchroPost.h"




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
	SetReplicateMovement(false);

	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	StateComponent = CreateDefaultSubobject<UStateComponent>(TEXT("StateComponent"));
	GridMoveComponent = CreateDefaultSubobject<UGridMoveComponent>(TEXT("GridMoveComponent"));

	HealthBarWidgetComponent = CreateDefaultSubobject<UUnitHealthBarComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(GetMesh());
	
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->SetRelativeLocation(
				FVector(0.0f, 0.0f, -CapsuleComp->GetScaledCapsuleHalfHeight()));
		}
	}
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	
	Super::BeginPlay();

	if (!CurrentUnitData && DefaultUnitData)
	{
		InitializeUnit(DefaultUnitData);
	}

	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->Refresh(this);
	}

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

	if (PresentationBehavior)
	{
		PresentationBehavior->TickPresentation(this, DeltaTime);
	}

	RefreshTickEnabled();
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


void AUnit::RefreshTickEnabled()
{
	const bool bNeedsTick = PresentationBehavior && PresentationBehavior->NeedsTick();

	SetActorTickEnabled(bNeedsTick);
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

	// Skeletal Mesh와 애니메이션 클래스 설정
	if (!CurrentUnitData->UnitMesh.IsNull())
	{
		GetMesh()->SetSkeletalMesh(CurrentUnitData->UnitMesh.LoadSynchronous());
		GetMesh()->SetRelativeRotation(CurrentUnitData->MeshRotationOffset);
		if (const UUnitAnimSetDataAsset* AnimSet = CurrentUnitData->AnimSet)
		{
			if (!AnimSet->AnimClass.IsNull())
			{
				GetMesh()->SetAnimInstanceClass(AnimSet->AnimClass.LoadSynchronous());
			}
		}
	}
	
	// 부착물 부착하기
	RebuildAttachments();
	
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
		HealthBarWidgetComponent->Refresh(this);
	}
}

void AUnit::SnapToTile(const FIntPoint& TileCoord)
{
	SetActorLocation(GetStandLocation(TileCoord));
}

FVector AUnit::GetStandLocation(const FIntPoint& Coord) const
{
	UGridManager* GridManager = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!GridManager)
	{
		return GetActorLocation();
	}
	FVector Location = GridManager->GetTileWorldLocation(Coord);
	if (const UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		Location.Z += CapsuleComp->GetScaledCapsuleHalfHeight();
	}
	return Location;
}

APlayerState* AUnit::GetControllingPlayerState() const
{
	const ASPGameState* SPGameState = GetWorld() ? GetWorld()->GetGameState<ASPGameState>() : nullptr;
	const UUnitSlotComponent* SlotComp = SPGameState ? SPGameState->GetUnitSlotComponent() : nullptr;

	return SlotComp ? SlotComp->GetOwnerOfUnit(this) : nullptr;
}

bool AUnit::IsControlledBy(const APlayerState* InPlayerState) const
{
	return InPlayerState != nullptr && GetControllingPlayerState() == InPlayerState;
}

bool AUnit::FindSocketLocation(FName SocketName, FVector& OutLocation) const
{
	if (SocketName.IsNone())
	{
		return false;
	}

	for (UMeshComponent* MeshComp : AttachmentComponents)
	{
		if (MeshComp && MeshComp->DoesSocketExist(SocketName))
		{
			OutLocation = MeshComp->GetSocketLocation(SocketName);
			return true;
		}
	}

	const USkeletalMeshComponent* BodyMesh = GetMesh();

	if (BodyMesh && BodyMesh->DoesSocketExist(SocketName))
	{
		OutLocation = BodyMesh->GetSocketLocation(SocketName);
		return true;
	}

	UE_LOG(LogSP, Warning, TEXT("[%s] 소켓 '%s'을(를) 부착물·몸 어디서도 못 찾음"), *GetName(), *SocketName.ToString());
	return false;
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

			EventComp->PushReactionEvent(Event);
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

			EventComp->PushReactionEvent(Event);
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
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->NotifyHealthPresented(DisplayAmount, NewTargetHealth, bIsCritical, TypeTags);
	}

	if (DisplayAmount < 0)
	{
		PresentHit();
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



void AUnit::OnRep_GridPosition(FIntPoint OldGridPosition)
{
	UE_LOG(LogSP, Warning, TEXT("[NET] OnRep_GridPosition %s: %s → %s"),
		*GetName(), *OldGridPosition.ToString(), *GridPosition.ToString());
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

void AUnit::PresentHit()
{
	if (PresentationBehavior)
	{
		PresentationBehavior->PresentHit(this);
	}
}

void AUnit::PresentMoveSegment(const FIntPoint& From, const FIntPoint& To)
{
	if (PresentationBehavior)
	{
		UE_LOG(LogSP, Log, TEXT("[%s] PresentMoveSegment: %s → %s"), *GetName(), *From.ToString(), *To.ToString());
		PresentationBehavior->PresentMoveSegment(this, From, To);
		RefreshTickEnabled();
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

void AUnit::SetGridPosition(const FIntPoint& NewPosition)
{
	UE_LOG(LogSP, Warning, TEXT("[NET] SetGridPosition %s (Auth=%d): %s → %s"),
		*GetName(), HasAuthority() ? 1 : 0,
		*GridPosition.ToString(), *NewPosition.ToString());

	GridPosition = NewPosition;
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

FGameplayTagContainer AUnit::GetStateTags() const
{
	return StateComponent ? StateComponent->GetStateTags() : FGameplayTagContainer();
}

const UUnitAnimSetDataAsset* AUnit::GetAnimSet() const
{
	return CurrentUnitData ? CurrentUnitData->AnimSet.Get() : nullptr;
}

UCombatEventComponent* AUnit::GetCombatEventComponent() const
{
	if (ASPGameState* GameState = GetWorld()->GetGameState<ASPGameState>())
	{
		return GameState->GetCombatEventComponent();
	}

	return nullptr;
}

void AUnit::SetHovered(bool bHovered)
{
	if (bIsHovered == bHovered)
	{
		return;
	}
	bIsHovered = bHovered;
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetHovered(bHovered);
	}
	
}

void AUnit::SetSelected(bool bSelected)
{
	if (bIsSelected == bSelected)
	{
		return;
	}
	bIsSelected = bSelected;
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetSelected(bSelected);
	}
}

void AUnit::RebuildAttachments()
{
	for (UMeshComponent* AttachmentComp : AttachmentComponents)
	{
		if (AttachmentComp)
		{
			AttachmentComp->DestroyComponent();
		}
	}
	AttachmentComponents.Empty();

	for (UUnitAttachment* Attachment : CurrentUnitData->Attachments)
	{
		if (Attachment)
		{
			UMeshComponent* NewAttachmentComp = Attachment->CreateAttachedComponent(this);
			if (NewAttachmentComp)
			{
				AttachmentComponents.Add(NewAttachmentComp);
			}
		}
		else
		{
			UE_LOG(LogSP, Warning, TEXT("[Attachment] 유닛 데이터에 NULL Attachment이 있습니다. %s"), *GetName());
		}
	}
}

bool AUnit::IsPresentingMove() const
{
	return PresentationBehavior && PresentationBehavior->IsPresentingMove();
}

float AUnit::GetPresentationMoveSpeed() const
{
	if (PresentationBehavior)
	{
		return PresentationBehavior->GetPresentationMoveSpeed();
	}
	return 0.0f;
}