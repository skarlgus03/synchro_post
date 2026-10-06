#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "StageMarkers.generated.h"

class UStaticMeshComponent;
class UArrowComponent;
class UStaticMesh;
class UObstacleDataAsset;

/*
* 레벨 위에 배치되는 마커용 액터 (에디터 전용)
* DA <-> 마커 번역 (Load/Bake)는 AStageBaker가 함.
*/
UCLASS(Abstract, HideCategories = (Physics, Collision, Input, LOD, Cooking, Replication, HLOD, Networking))
class SYNCHROPOST_API AStageMarker : public AActor
{
	GENERATED_BODY()
	
public:	
	AStageMarker();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;


#if WITH_EDITORONLY_DATA
	// 앞 방향 확인용 
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif

	// 마커용 기본 메시 (없으면 Arrow만 표시됨)
	UPROPERTY()
	TObjectPtr<UStaticMesh> FallbackMesh;

	void SetPreviewMesh(UStaticMesh* Mesh);
};


// Transform 마커 : 액터 Transform이 그대로 슬롯 Transform

UCLASS()
class SYNCHROPOST_API AStagePropMarker : public AStageMarker
{
	GENERATED_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Marker")
	FGameplayTag SlotType;

	UPROPERTY(EditAnywhere, Category = "Marker")
	TSoftObjectPtr<UStaticMesh> Mesh;
};

UCLASS()
class SYNCHROPOST_API AStageEntityMarker : public AStageMarker
{
	GENERATED_BODY()

public:
	virtual void OnConstruction(const FTransform& Tranform) override;

	UPROPERTY(EditAnywhere, Category = "Marker")
	FGameplayTag SlotType;

	UPROPERTY(EditAnywhere, Category = "Marker")
	TSoftClassPtr<AActor> ActorClass;
};


UCLASS()
class SYNCHROPOST_API AStageExitMarker : public AStageMarker
{
	GENERATED_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Marker")
	FGameplayTag Position;

	UPROPERTY(EditAnywhere, Category = "Marker")
	TSoftObjectPtr<UStaticMesh> Mesh;
};	

// ---------- 그리드 마커: 위치는 칸, 회전·스케일은 외형 ----------

UCLASS()
class SYNCHROPOST_API AGridObstacleMarker : public AStageMarker
{
	GENERATED_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// 드래그를 놓으면 같은 레벨 베이커의 칸 중심으로 스냅
	virtual void PostEditMove(bool bFinished) override;
#endif

	UPROPERTY(EditAnywhere, Category = "Marker")
	TObjectPtr<UObstacleDataAsset> ObstacleData;
};