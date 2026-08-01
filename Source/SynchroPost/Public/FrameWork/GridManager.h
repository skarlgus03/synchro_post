
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Grid/Tile.h"
#include "Types/SPCombatEventStructure.h"
#include "Interface/TileTrigger.h"
#include "GridManager.generated.h"

class UTileMapDataAsset;
class AUnit;

USTRUCT()
struct FTileTriggerList
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TScriptInterface<ITileTrigger>> Triggers;
};

USTRUCT(BlueprintType)
struct FGridReachability
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TMap<FIntPoint, FIntPoint> CameFrom;

    UPROPERTY(BlueprintReadOnly)
    TMap<FIntPoint, int32> DistanceFromStart;
};

UCLASS()
class SYNCHROPOST_API UGridManager : public UWorldSubsystem
{
	GENERATED_BODY()
	

public:

    // 스테이지 시작시, 호출
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void LoadGrid(UTileMapDataAsset* StageData);

	// 좌표에 있는 유닛을 반환. 없으면 nullptr 반환
    UFUNCTION(BlueprintCallable, Category = "Grid")
    AUnit* GetUnitAt(const FIntPoint& Coord) const;

	// 좌표가 이동 가능한지 확인. 이동 불가하면 false 반환
    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool IsWalkable(const FIntPoint& Coord) const;



	// 좌표에 유닛을 배치. 이미 유닛이 있으면 덮어씀
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetUnitAt(const FIntPoint& Coord, AUnit* Unit);

	// 좌표에 있는 유닛 제거
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ClearUnitAt(const FIntPoint& Coord);
    
	// 좌표에 있는 타일의 타입을 변경
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetTileType(const FIntPoint& Coord, ETileType NewType);

	
    
    // 유닛의 좌표를 즉시 이동시킨다. 내부적으로 SetUnitAt, ClearUnitAt를 호출한다.
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void MoveUnitAt(const FIntPoint& ToCoord, AUnit* Unit);
    
	// 유닛을 경로를 따라 이동시키고, 이동중에 트리거가 발생할 경우, 트리거를 실행하고 결과를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FMoveStep> MoveUnitAlongPath(AUnit* Unit, const TArray<FIntPoint>& Path);



	// 특정 좌표에 트리거를 하나 추가한다. 이미 같은 트리거가 있으면 무시
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void AddTileTrigger(const FIntPoint& Coord, TScriptInterface<ITileTrigger> Trigger);

    // 특정 좌표에서 특정 트리거를 하나 제거한다.
	UFUNCTION(BlueprintCallable, Category = "Grid")
    void RemoveTileTrigger(const FIntPoint& Coord, TScriptInterface<ITileTrigger> Trigger);

    // 특정 좌표에서 MaxRange만큼 이동 가능한 좌표들을 반환한다. 
	UFUNCTION(BlueprintCallable, Category = "Grid")
    FGridReachability GetReachableTiles(const FIntPoint& Start, int32 MaxRange) const;

	// A* 알고리즘을 이용하여, Start에서 Destination까지의 경로를 반환한다. Tie-breaker는 목적지 방향으로의 직선 이탈 정도를 고려한다. MaxRange를 초과하면 빈 배열 반환
    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<FIntPoint> FindPath(const FIntPoint& Start, const FIntPoint& Destination, int32 MaxRange) const;
    
    
	UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GetTileWorldLocation(const FIntPoint& Coord) const;

    const TArray<FTile>& GetAllTiles() const { return TileGrid.Entries; }

	UFUNCTION(BlueprintCallable, Category = "Grid")
	FIntPoint WorldLocationToCoord(const FVector& WorldLocation) const;

    UFUNCTION()
	void HandleUnitDied(AUnit* Unit);


protected:

    UPROPERTY()
    FTileGrid TileGrid;

    UPROPERTY()
    TMap<FIntPoint, FTileTriggerList> TileTriggers;
};
