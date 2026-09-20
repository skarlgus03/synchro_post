#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPPlayerController.generated.h"

class AGridVisualizer;
class UTileMapDataAsset;
class AUnit;
class UNodeSelectionWidget;
class UGridActionMode;
class UCombatActionWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedUnitChanged, AUnit*, NewSelectedUnit);

UCLASS()
class SYNCHROPOST_API ASPPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	ASPPlayerController();
	virtual void BeginPlay() override;

	/*
	* 좌클릭 단일 진입점. 액션 모드면 행동 확정, 혹은 정보 조회용 선택/해제 
	* BP 입력에서 ConfirmAction 대신 이것을 호출합니다.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input")
	void HandlePrimaryClick();

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnit(AUnit* NewSelectedUnit);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	AUnit* GetSelectedUnit() const { return SelectedUnit.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Selection")
	AUnit* GetHoveredUnit() const { return HoveredUnit.Get(); }


	UFUNCTION(BlueprintCallable, Category = "Action Mode")
	void EnterMoveMode();
	
	UFUNCTION(BlueprintCallable, Category = "Action Mode")
	void EnterSkillMode(FGameplayTag SkillSlotTag);

	UFUNCTION(BlueprintCallable, Category = "Action Mode")
	void ExitActionMode();

	UFUNCTION(BlueprintCallable, Category = "Action Mode")
	void ConfirmAction();


	UFUNCTION()
	void HandleTileGridUpdated();



	UFUNCTION(Client, Reliable)
	void Client_LoadStageLevel(const TSoftObjectPtr<UWorld>& LevelAsset);

	UFUNCTION(Client, Reliable)
	void Client_ShowNodeSelection();

	UFUNCTION(Server, Reliable)
	void Server_RequestEnterNode(int32 NodeIndex);

	UFUNCTION(Server, Reliable)
	void Server_RequestEndTurn();

	UFUNCTION(Server, Reliable)
	void Server_NotifyClientReady();

	bool IsReadyForStageData() const { return bIsReadyForStageData; }

	// 디버그


	// Getter

	FORCEINLINE AGridVisualizer* GetGridVisualizer() const { return GridVisualizer; }
	
	// 선택이 바뀔 때 알린다.
	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnSelectedUnitChanged OnSelectedUnitChanged;

protected:

	virtual void Tick(float DeltaSeconds) override;

	// 커서 아래의 좌표와 유닛을 추적한다. 액션 모드와 무관함
	void UpdateCursorTarget();

	// 액션 모드의 타일 프리뷰를 갱신한다. 모드가 있을때만 호출
	void UpdateActionModePreview(const FIntPoint& PreviousCoord);

	// 호버 유닛이 바뀌었을 때 이전/새 유닛에 알린다.
	void SetHoveredUnit(AUnit* NewHoveredUnit);




	void EnterActionMode(UGridActionMode* NewMode);

	UFUNCTION()
	void HandleUnitTurnStart(AUnit* Unit);

	UFUNCTION()
	void HandleUnitTurnEnd(AUnit* Unit);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Grid Visualizer")
	TSubclassOf<AGridVisualizer> GridVisualizerClass;

	UPROPERTY()
	TObjectPtr<AGridVisualizer> GridVisualizer;

	FVector2D LastMouseScreenPosition = FVector2D(-1.0f, -1.0f);

	UPROPERTY()
	TObjectPtr<UGridActionMode> ActiveActionMode;

	UPROPERTY()
	TArray<FIntPoint> CachedRangeTiles;

	UPROPERTY()
	TArray<FIntPoint> CachedRelatedTiles;

	FIntPoint LastHoveredCoord = FIntPoint(MIN_int32, MIN_int32);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNodeSelectionWidget> NodeSelectionWidgetClass;

	UPROPERTY()
	TObjectPtr<UNodeSelectionWidget> NodeSelectionWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCombatActionWidget> CombatActionWidgetClass;

	UPROPERTY()
	TObjectPtr<UCombatActionWidget> CombatActionWidgetInstance;

	bool bIsReadyForStageData = false;


	TWeakObjectPtr<AUnit> SelectedUnit;

	TWeakObjectPtr<AUnit> HoveredUnit;
};
