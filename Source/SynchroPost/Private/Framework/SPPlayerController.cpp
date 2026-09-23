#include "Framework/SPPlayerController.h"
#include "Framework/GridVisualizer.h"
#include "Grid/TileMapDataAsset.h"
#include "Unit/Unit.h"
#include "Unit/GridMoveComponent.h"
#include "Framework/TurnManager.h"
#include "Framework/GridManager.h"
#include "UI/NodeSelectionWidget.h"
#include "Framework/RunProgressSubsystem.h"
#include "Grid/GridActionMode.h"
#include "UI/CombatActionWidget.h"
#include "Framework/StageGameMode.h"
#include "Grid/GridStateComponent.h"
#include "Framework/SPGameState.h"
#include "Framework/TurnStateComponent.h"
#include "Slot/UnitSlotComponent.h"
#include "GameFramework/PlayerState.h"
#include "SynchroPost.h"
#include "Unit/SkillComponent.h"


ASPPlayerController::ASPPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (GridVisualizerClass)
		{
			GridVisualizer = GetWorld()->SpawnActor<AGridVisualizer>(GridVisualizerClass);
			if (GridVisualizer)
			{
				// 이미 데이터가 와 있는 경우
				GridVisualizer->PopulateFromGrid();
			}
		}
		if (ASPGameState* SPGameState = GetWorld()->GetGameState<ASPGameState>())
		{
			if (UTurnStateComponent* TurnState = SPGameState->GetTurnStateComponent())
			{
				TurnState->OnUnitTurnStart.AddDynamic(this, &ASPPlayerController::HandleUnitTurnStart);
				TurnState->OnUnitTurnEnd.AddDynamic(this, &ASPPlayerController::HandleUnitTurnEnd);
			}
		}
		if (ASPGameState* SPGameState = GetWorld()->GetGameState<ASPGameState>())
		{
			if (UGridStateComponent* GridState = SPGameState->GetGridStateComponent())
			{
				GridState->OnTileGridUpdated.AddDynamic(this, &ASPPlayerController::HandleTileGridUpdated);
			}
		}

		// 클라이언트가 스테이지 데이터를 받을 준비가 되었음을 서버에 알림
		Server_NotifyClientReady();
	}


	bShowMouseCursor = true;

}

void ASPPlayerController::HandlePrimaryClick()
{
	// 좌클릭의 의미는 모드가 정함
	if (ActiveActionMode)
	{
		ConfirmAction();
		return;
	}

	// 액션 모드가 아니면 정보 조회용 선택임. 빈 곳을 클릭하면 nullptr->해제
	SelectUnit(HoveredUnit.Get());
}

void ASPPlayerController::SelectUnit(AUnit* NewSelectedUnit)
{
	AUnit* Previous = SelectedUnit.Get();

	// 같은 대상이면 아무것도 안함
	if (Previous == NewSelectedUnit)
	{
		return;
	}
	
	if (Previous)
	{
		Previous->SetSelected(false);
	}

	SelectedUnit = NewSelectedUnit;

	if (NewSelectedUnit)
	{
		NewSelectedUnit->SetSelected(true);
	}

	OnSelectedUnitChanged.Broadcast(NewSelectedUnit);
}

void ASPPlayerController::EnterMoveMode()
{
	AUnit* CurrentUnit = GetActingUnit();
	if (!CurrentUnit || !CurrentUnit->IsControlledBy(PlayerState))
	{
		return;   // 남의 유닛이면 헛된 UI를 띄우지 않는다. 서버도 거부한다
	}

	UMoveActionMode* MoveMode = NewObject<UMoveActionMode>(this);
	MoveMode->Initialize(CurrentUnit, this);
	EnterActionMode(MoveMode);
}

void ASPPlayerController::EnterSkillMode(FGameplayTag SkillSlotTag)
{
	AUnit* CurrentUnit = GetActingUnit();
	if (!CurrentUnit || !CurrentUnit->IsControlledBy(PlayerState))
	{
		return;
	}

	USkillActionMode* SkillMode = NewObject<USkillActionMode>(this);
	SkillMode->Initialize(CurrentUnit, this);
	SkillMode->SetSkillSlotTag(SkillSlotTag);
	EnterActionMode(SkillMode);
}

void ASPPlayerController::ExitActionMode()
{
	if (GridVisualizer)
	{
		GridVisualizer->RemoveTileStates(CachedRangeTiles, ETileVisualState::InRange);
		GridVisualizer->RemoveTileStates(CachedRelatedTiles, ETileVisualState::OnPath);
		if (LastHoveredCoord != FIntPoint(MIN_int32, MIN_int32))
		{
			GridVisualizer->RemoveTileState(LastHoveredCoord, ETileVisualState::Hovered);
			GridVisualizer->RemoveTileState(LastHoveredCoord, ETileVisualState::ValidTarget);
		}
	}
	ActiveActionMode = nullptr;
	CachedRangeTiles.Empty();
	CachedRelatedTiles.Empty();

}

void ASPPlayerController::ConfirmAction()
{
	UE_LOG(LogTemp, Warning, TEXT("ConfirmAction 호출됨 Coord = %s"), *LastHoveredCoord.ToString());

	if (!ActiveActionMode)
	{
		return;
	}
	if (!ActiveActionMode->IsValidTarget(LastHoveredCoord))
	{
		return;
	}
	const bool bSelectionComplete = ActiveActionMode->RegisterSelection(LastHoveredCoord);
	if (!bSelectionComplete)
	{
		return;
	}

	ActiveActionMode->ConfirmAction(LastHoveredCoord);
	ExitActionMode();
}

void ASPPlayerController::HandleTileGridUpdated()
{
	if (GridVisualizer)
	{
		GridVisualizer->PopulateFromGrid();
	}
}

AUnit* ASPPlayerController::GetActingUnit() const
{
	const ASPGameState* SPGameState = GetWorld()->GetGameState<ASPGameState>();
	const UTurnStateComponent* TurnState = SPGameState ? SPGameState->GetTurnStateComponent() : nullptr;
	return TurnState ? TurnState->GetCurrentUnit() : nullptr;
}

void ASPPlayerController::Server_RequestMove_Implementation(AUnit* Unit, const FIntPoint& Destination)
{
	if (!CanCommandUnit(Unit))
	{
		return;
	}
	if (UGridMoveComponent* MoveComp = Unit->GetGridMoveComponent())
	{
		MoveComp->RequestMove(Destination);
	}
}

void ASPPlayerController::Server_ExecuteSkill_Implementation(AUnit* Unit, const FGameplayTag& SkillSlotTag, const FSkillTargetData& Target)
{
	if (!CanCommandUnit(Unit))
	{
		return;
	}
	if (USkillComponent* SkillComp = Unit->GetSkillComponent())
	{
		SkillComp->ExecuteSkill(SkillSlotTag, Target);
	}

}

void ASPPlayerController::Client_LoadStageLevel_Implementation(const TSoftObjectPtr<UWorld>& LevelAsset)
{
	if (URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>())
	{
		RunProgress->LoadStageLevelForClient(LevelAsset);
	}
}



void ASPPlayerController::Client_ShowNodeSelection_Implementation()
{
	if (!NodeSelectionWidgetClass) return;

	if (!NodeSelectionWidgetInstance)
	{
		NodeSelectionWidgetInstance = CreateWidget<UNodeSelectionWidget>(this, NodeSelectionWidgetClass);
	}

	if (NodeSelectionWidgetInstance)
	{
		NodeSelectionWidgetInstance->RefreshNodeButtons();
		if (!NodeSelectionWidgetInstance->IsInViewport())
		{
			NodeSelectionWidgetInstance->AddToViewport();
		}
	}
}

void ASPPlayerController::Server_RequestEnterNode_Implementation(int32 NodeIndex)
{
	if (URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>(); RunProgress)
	{
		// 투표제 생성해야함
		RunProgress->EnterNode(NodeIndex);
	}
}

void ASPPlayerController::Server_RequestEndTurn_Implementation()
{
	if (UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>())
	{
		TurnManager->EndCurrentUnitTurn();
	}
}

void ASPPlayerController::Server_NotifyClientReady_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Server_NotifyClientReady_Implementation 호출됨: %s"), *GetName());
	if (bIsReadyForStageData)
	{
		return;
	}
	bIsReadyForStageData = true;

	if (ASPGameState* SPGameState = GetWorld()->GetGameState<ASPGameState>())
	{
		if (UUnitSlotComponent* UnitSlotcomp = SPGameState->GetUnitSlotComponent())
		{
			UnitSlotcomp->DistributeSlotsEvenly(SPGameState->PlayerArray);
		}
	}

	if (AStageGameMode* StageGameMode = GetWorld()->GetAuthGameMode<AStageGameMode>())
	{
		StageGameMode->SendCurrentStageDataToPlayer(this);
	}
}

void ASPPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocalController())
	{
		UpdateCursorTarget();
	}
}

void ASPPlayerController::UpdateCursorTarget()
{
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}
	const FVector2D CurrentMousePos(MouseX, MouseY);
	if (CurrentMousePos.Equals(LastMouseScreenPosition, 0.5f))
	{
		return;
	}
	LastMouseScreenPosition = CurrentMousePos;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		SetHoveredUnit(nullptr);
		return;
	}

	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager)
	{
		return;
	}
		
	AUnit* NewHoveredUnit = Cast<AUnit>(Hit.GetActor());
	FIntPoint NewCoord;

	if (NewHoveredUnit)
	{
		NewCoord = NewHoveredUnit->GetGridPosition();
	}
	else
	{
		NewCoord = GridManager->WorldLocationToCoord(Hit.Location);
		NewHoveredUnit = GridManager->GetUnitAt(NewCoord);
	}

	SetHoveredUnit(NewHoveredUnit);

	// (2) 좌표가 바뀌었으면 액션 모드 프리뷰 갱신
	if (NewCoord == LastHoveredCoord)
	{
		return;
	}

	const FIntPoint PreviousCoord = LastHoveredCoord;
	LastHoveredCoord = NewCoord;

	if (ActiveActionMode)
	{
		UpdateActionModePreview(PreviousCoord);
	}
}

void ASPPlayerController::UpdateActionModePreview(const FIntPoint& PreviousCoord)
{
	if (!ActiveActionMode || !GridVisualizer)
	{
		return;
	}

	// 이전 프리뷰 정리
	if (CachedRelatedTiles.Num() > 0)
	{
		GridVisualizer->RemoveTileStates(CachedRelatedTiles, ETileVisualState::OnPath);
		CachedRelatedTiles.Empty();
	}
	if (PreviousCoord != FIntPoint(MIN_int32, MIN_int32))
	{
		GridVisualizer->RemoveTileState(PreviousCoord, ETileVisualState::Hovered);
		GridVisualizer->RemoveTileState(PreviousCoord, ETileVisualState::ValidTarget);
	}

	// 이미 범위 밖이면 아무것도 하지 않는다.
	if (!CachedRangeTiles.Contains(LastHoveredCoord))
	{
		return;   
	}

	// 유효한 타겟일 때만 ValidTarget을 칠한다.
	if (!ActiveActionMode->IsValidTarget(LastHoveredCoord))
	{
		return;
	}

	// 새로운 프리뷰 적용
	GridVisualizer->AddTileState(LastHoveredCoord, ETileVisualState::ValidTarget);

	CachedRelatedTiles = ActiveActionMode->ComputeRelatedTiles(LastHoveredCoord);
	GridVisualizer->AddTileStates(CachedRelatedTiles, ETileVisualState::OnPath);
}

void ASPPlayerController::SetHoveredUnit(AUnit* NewHoveredUnit)
{
	AUnit* Previous = HoveredUnit.Get();
	if (Previous == NewHoveredUnit)
	{
		return;
	}

	if (Previous)
	{
		Previous->SetHovered(false);
	}

	HoveredUnit = NewHoveredUnit;

	if (NewHoveredUnit)
	{
		NewHoveredUnit->SetHovered(true);
	}
}

void ASPPlayerController::EnterActionMode(UGridActionMode* NewMode)
{
	ExitActionMode();

	UE_LOG(LogSP, Warning, TEXT("[Mode] EnterActionMode | Mode=%s GridVisualizer=%d"),
		*GetNameSafe(NewMode), GridVisualizer ? 1 : 0);


	if (!NewMode || !GridVisualizer)
	{
		return;
	}

	ActiveActionMode = NewMode;
	CachedRangeTiles = ActiveActionMode->GetRangeTiles();

	GridVisualizer->AddTileStates(CachedRangeTiles, ETileVisualState::InRange);

	UpdateActionModePreview(FIntPoint(MIN_int32, MIN_int32));
}

void ASPPlayerController::HandleUnitTurnStart(AUnit* Unit)
{
	RefreshCombatActionWidget();
}

void ASPPlayerController::HandleUnitTurnEnd(AUnit* Unit)
{
	ExitActionMode();
	RefreshCombatActionWidget();
}

bool ASPPlayerController::CanCommandUnit(const AUnit* Unit) const
{
	if (!Unit)
	{
		UE_LOG(LogSP, Warning, TEXT("[Cmd] 거부 - 유닛이 null"));
		return false;
	}

	// 소유권: 자기가 담당하는 유닛만
	if (!Unit->IsControlledBy(PlayerState))
	{
		UE_LOG(LogSP, Warning, TEXT("[Cmd] 거부 - 소유권 없음 | 요청자=%d 담당=%d 유닛=%s"),
			PlayerState ? PlayerState->GetPlayerId() : -1,
			Unit->GetControllingPlayerState() ? Unit->GetControllingPlayerState()->GetPlayerId() : -1,
			*Unit->GetName());
		return false;
	}

	// 턴: 지금 그 유닛의 턴이어야 한다.
	// 이게 없으면 클라가 자기 유닛에게 아무 때나 명령할 수 있다.
	if (GetActingUnit() != Unit)
	{
		UE_LOG(LogSP, Warning, TEXT("[Cmd] 거부 - 해당 유닛의 턴이 아님 | 유닛=%s"), *Unit->GetName());
		return false;
	}

	return true;
}

void ASPPlayerController::RefreshCombatActionWidget()
{
	if (!IsLocalController() || !CombatActionWidgetClass)
	{
		return;
	}

	AUnit* ActingUnit = GetActingUnit();
	const bool bShouldShow = ActingUnit && ActingUnit->IsControlledBy(PlayerState);

	if (!bShouldShow)
	{
		if (CombatActionWidgetInstance && CombatActionWidgetInstance->IsInViewport())
		{
			CombatActionWidgetInstance->RemoveFromParent();
		}
		return;
	}

	if (!CombatActionWidgetInstance)
	{
		CombatActionWidgetInstance = CreateWidget<UCombatActionWidget>(this, CombatActionWidgetClass);
	}
	if (CombatActionWidgetInstance && !CombatActionWidgetInstance->IsInViewport())
	{
		CombatActionWidgetInstance->AddToViewport();
	}
}

void ASPPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 턴 이벤트가 PlayerState보다 먼저 도착할 수 있음.
	RefreshCombatActionWidget();
}
