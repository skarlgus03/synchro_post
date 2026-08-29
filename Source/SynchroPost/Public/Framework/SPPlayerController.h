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

UCLASS()
class SYNCHROPOST_API ASPPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	ASPPlayerController();
	virtual void BeginPlay() override;


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

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugKillHoveredUnit();


	// Getter

	FORCEINLINE AGridVisualizer* GetGridVisualizer() const { return GridVisualizer; }
	
protected:

	virtual void Tick(float DeltaSeconds) override;

	void UpdateHoverTile();

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
};
