
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Grid/Tile.h"
#include "GridManager.generated.h"

class UTileMapDataAsset;
class AUnit;

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

	// 유닛의 좌표를 이동시킨다. 내부적으로 SetUnitAt, ClearUnitAt를 호출한다.
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void MoveUnitAt(const FIntPoint& ToCoord, AUnit* Unit);

    const TArray<FTile>& GetAllTiles() const { return TileGrid.Entries; }



    UFUNCTION()
	void HandleUnitDied(AUnit* Unit);


protected:

    UPROPERTY()
    FTileGrid TileGrid;


};
