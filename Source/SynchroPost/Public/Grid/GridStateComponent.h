

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

protected:

	UPROPERTY(ReplicatedUsing = OnRep_TileGrid)
	FTileGrid TileGrid;

	UFUNCTION()
	void OnRep_TileGrid();
};

