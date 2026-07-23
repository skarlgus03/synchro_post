#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/Tile.h"
#include "TileMapDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FTileSpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tile")
	FIntPoint Coordinate = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Tile")
	ETileType TileType = ETileType::Normal;
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
};
