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
		}
		if (UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>())
		{
			TurnManager->OnUnitTurnStart.AddDynamic(this, &ASPPlayerController::HandleUnitTurnStart);
			TurnManager->OnUnitTurnEnd.AddDynamic(this, &ASPPlayerController::HandleUnitTurnEnd);
			UE_LOG(LogTemp, Warning, TEXT("PC BeginPlay: OnUnitTurnStart 바인딩 완료"));
		}

		// 클라이언트가 스테이지 데이터를 받을 준비가 되었음을 서버에 알림
		Server_NotifyClientReady();
	}


	bShowMouseCursor = true;

}

void ASPPlayerController::EnterMoveMode()
{
	UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>();
	AUnit* CurrentUnit = TurnManager ? TurnManager->GetCurrentUnit() : nullptr;
	if (!CurrentUnit)
	{
		return;
	}

	UMoveActionMode* MoveMode = NewObject<UMoveActionMode>(this);
	MoveMode->Initialize(CurrentUnit);
	EnterActionMode(MoveMode);
}

void ASPPlayerController::EnterSkillMode(FGameplayTag SkillSlotTag)
{
	UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>();
	AUnit* CurrentUnit = TurnManager ? TurnManager->GetCurrentUnit() : nullptr;
	if (!CurrentUnit)
	{
		return;
	}

	USkillActionMode* SkillMode = NewObject<USkillActionMode>(this);
	SkillMode->Initialize(CurrentUnit);
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
	LastHoveredCoord = FIntPoint(MIN_int32, MIN_int32);
}

void ASPPlayerController::ConfirmAction()
{
	if (!ActiveActionMode)
	{
		return;
	}
	if (!ActiveActionMode->IsValidTarget(LastHoveredCoord))
	{
		return;
	}
	ActiveActionMode->ConfirmAction(LastHoveredCoord);
	ExitActionMode();
}

void ASPPlayerController::Client_LoadStageLevel_Implementation(const TSoftObjectPtr<UWorld>& LevelAsset)
{
	if (URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>())
	{
		RunProgress->LoadStageLevelForClient(LevelAsset);
	}
}

void ASPPlayerController::Client_InitializeCombatGrid_Implementation(UTileMapDataAsset* TileMapData)
{
	if (!HasAuthority())
	{
		if (UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>())
		{
			GridManager->LoadGrid(TileMapData);
		}
	}

	if (GridVisualizer)
	{
		GridVisualizer->PopulateFromGrid();
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

	if (AStageGameMode* StageGameMode = GetWorld()->GetAuthGameMode<AStageGameMode>())
	{
		StageGameMode->SendCurrentStageDataToPlayer(this);
	}
}

void ASPPlayerController::DebugKillHoveredUnit()
{
	UGridManager* GridManger = GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManger)
	{
		return;
	}

	AUnit* Target = GridManger->GetUnitAt(LastHoveredCoord);
	
	if (!Target) return;

	FSPDamageData DamageData;
	DamageData.RawDamage = 9999;
	DamageData.DamageCauser = this->GetPawn();

	UE_LOG(LogTemp, Warning, TEXT("DebugKillHoveredUnit: Applying %d damage to %s"), DamageData.RawDamage, *Target->GetName());
	Target->ApplyDamage(DamageData);
}

void ASPPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateHoverTile();
}

void ASPPlayerController::UpdateHoverTile()
{
	if (!ActiveActionMode)
	{
		return;
	}

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
	if (!GridVisualizer)
	{
		return;
	}
	UGridManager* CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!CachedGridManager)
	{
		return;
	}
	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		return;
	}


	const FIntPoint HoveredCoord = CachedGridManager->WorldLocationToCoord(Hit.Location);
	if (HoveredCoord == LastHoveredCoord)
	{
		return;
	}

	// 이전 경로 상태 정리 (OnPath 제거, 마지막 타일의 Hovered도 제거)
	if (CachedRelatedTiles.Num() > 0)
	{
		GridVisualizer->RemoveTileStates(CachedRelatedTiles, ETileVisualState::OnPath);
		CachedRelatedTiles.Empty();
	}
	if (LastHoveredCoord != FIntPoint(MIN_int32, MIN_int32))
	{
		GridVisualizer->RemoveTileState(LastHoveredCoord, ETileVisualState::Hovered);
		GridVisualizer->RemoveTileState(LastHoveredCoord, ETileVisualState::ValidTarget);
	}

	LastHoveredCoord = HoveredCoord;

	if (!CachedRangeTiles.Contains(HoveredCoord))
	{
		return; // 범위 밖이면 여기서 끝
	}
	GridVisualizer->AddTileState(HoveredCoord, ETileVisualState::ValidTarget);
	if (!ActiveActionMode->IsValidTarget(HoveredCoord))
	{
		return; // 유효한 타겟이 아니면 여기서 끝
	}
	
	GridVisualizer->AddTileState(HoveredCoord, ETileVisualState::ValidTarget);

	CachedRelatedTiles = ActiveActionMode->ComputeRelatedTiles(HoveredCoord);
	GridVisualizer->AddTileStates(CachedRelatedTiles, ETileVisualState::OnPath);
	
}

void ASPPlayerController::EnterActionMode(UGridActionMode* NewMode)
{
	ExitActionMode();

	if (!NewMode || !GridVisualizer)
	{
		return;
	}

	ActiveActionMode = NewMode;
	CachedRangeTiles = ActiveActionMode->GetRangeTiles();

	GridVisualizer->AddTileStates(CachedRangeTiles, ETileVisualState::InRange);
}

void ASPPlayerController::HandleUnitTurnStart(AUnit* Unit)
{
	if (!CombatActionWidgetClass)
	{
		return;
	}

	
	if(!CombatActionWidgetInstance)
	{
		CombatActionWidgetInstance = CreateWidget<UCombatActionWidget>(this, CombatActionWidgetClass);
	}
	if (CombatActionWidgetInstance)
	{
		if (!CombatActionWidgetInstance->IsInViewport())
		{
			CombatActionWidgetInstance->AddToViewport();
		}
	}
}

void ASPPlayerController::HandleUnitTurnEnd(AUnit* Unit)
{
	ExitActionMode();

	if (CombatActionWidgetInstance)
	{
		CombatActionWidgetInstance->RemoveFromParent();
	}
}