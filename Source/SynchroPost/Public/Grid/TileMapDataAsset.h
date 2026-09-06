#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/Tile.h"
#include "TileMapDataAsset.generated.h"

class UObstacleDataAsset;

USTRUCT(BlueprintType)
struct FTileSpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tile")
	FIntPoint Coordinate = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Tile")
	ETileType TileType = ETileType::Normal;
};

USTRUCT(BlueprintType)
struct FObstacleSpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Obstacle")
	FIntPoint Coordinate = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Obstacle")
	TObjectPtr<UObstacleDataAsset> ObstacleData;
};

UCLASS()
class SYNCHROPOST_API UTileMapDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 GridHeight = 10;

	// 타일 하나의 실제 크기
	UPROPERTY(EditAnywhere, Category = "Grid")
	float TileSize = 100.0f;

	// 기본값이 아닌 타일들을 이곳에 등록
	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FTileSpawnInfo> SpecialTiles;

	// 영구적으로 벽 타일로 설정할 좌표들. (이 좌표들은 항상 벽으로 간주됨)
	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FIntPoint> WallTiles;

	// 장애물 스폰 정보
	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FObstacleSpawnInfo> Obstacles;
};
