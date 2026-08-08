
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Types/SPGameFlowStructure.h"
#include "RunProgressSubsystem.generated.h"

class UFloorDataAsset;

UCLASS()
class SYNCHROPOST_API URunProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	int32 CurrentFloorIndex = 0;

	UPROPERTY()
	TObjectPtr<UFloorDataAsset> CurrentFloor;

	// 절차 생성된 노드 그래프를 저장하는 배열
	UPROPERTY()
	TArray<FStageNode> ResolvedNodeGraph;

	UPROPERTY()
	int32 CurrentNodeIndex = 0;
};
