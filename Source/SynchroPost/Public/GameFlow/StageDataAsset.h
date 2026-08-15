#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPGameFlowStructure.h"
#include "StageDataAsset.generated.h"

class UTileMapDataAsset;

UCLASS(Abstract)
class SYNCHROPOST_API UStageDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, Category = "Stage")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, Category = "Stage")
	TArray<FPropSlotInfo> PropSlots;

	UPROPERTY(EditAnywhere, Category = "Stage")
	TArray<FEntitySlotInfo> EntitySlots;

	UPROPERTY(EditAnywhere, Category = "Stage")
	TArray<FExitSlotInfo> ExitSlots;
};


UCLASS()
class SYNCHROPOST_API UCombatStageDataAsset : public UStageDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UTileMapDataAsset> TileMap;
};

UCLASS()
class SYNCHROPOST_API UEventStageDataAsset : public UStageDataAsset
{
	GENERATED_BODY()
	
	// 아직 안함
};

UCLASS()
class SYNCHROPOST_API URestStageDataAsset : public UStageDataAsset
{
	GENERATED_BODY()
	// 아직 안함
};

UCLASS()
class SYNCHROPOST_API UShopStageDataAsset : public UStageDataAsset
{
	GENERATED_BODY()
	// 아직 안함
};


UCLASS()
class SYNCHROPOST_API UUpgradeStageDataAsset : public UStageDataAsset
{
	GENERATED_BODY()
	// 아직 안함
};


UCLASS()
class SYNCHROPOST_API UDraftStageDataAsset : public UStageDataAsset
{
	GENERATED_BODY()
	// 아직 안함
};