

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/Tile.h"
#include "GridStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTileGridUpdate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UGridStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UGridStateComponent();
	
	FTileGrid& GetTileGridMutable() { return TileGrid; }
	const FTileGrid& GetTileGrid() const { return TileGrid; }
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable)
	FOnTileGridUpdate OnTileGridUpdated;

	// 서버 전용. 그리드를 생성하고 초기화함.
	void InitializeGrid(int32 Width, int32 Height, float TileSize);

protected:

	UPROPERTY(ReplicatedUsing = OnRep_TileGrid)
	FTileGrid TileGrid;

	UFUNCTION()
	void OnRep_TileGrid();

	/** 그리드 치수의 단일 출처. TileGrid 안의 GridWidth/Height/TileSize는 이것의 캐시다. */
	UPROPERTY(ReplicatedUsing = OnRep_GridDimensions)
	FIntPoint GridDimensions = FIntPoint::ZeroValue;

	UPROPERTY(ReplicatedUsing = OnRep_GridDimensions)
	float GridTileSize = 100.0f;

	UFUNCTION()
	void OnRep_GridDimensions();

private:

	void TryBroadcastGridUpdated();
};

