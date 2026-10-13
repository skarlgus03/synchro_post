#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "StageMarkers.generated.h"

class UStaticMeshComponent;
class UArrowComponent;
class UStaticMesh;
class UObstacleDataAsset;
class UBoxComponent;

UENUM()
enum class ESpawnVolumeTeam : uint8
{
	Ally UMETA(DisplayName = "아군"),
	Enemy UMETA(DisplayName = "적"),
};

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

/*
* 스폰 칸을 정하는 볼륨. 칸 중심이 박스 안이면 그 팀의 스폰칸이 된다.
* 볼륨은 범위와 팀만 가지고, 어느 칸이 스폰인지는 베이커가 판정함.
*/
UCLASS()
class SYNCHROPOST_API AGridSpawnVolume : public AStageMarker
{
	GENERATED_BODY()

public:
	AGridSpawnVolume();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** 이 위치(칸 중심)가 박스 안인가. XY만 보고, 회전·스케일은 반영된다. 경계는 포함하지 않는다 */
	bool ContainsXY(const FVector& WorldLocation) const;

	/** 칸 하나를 덮는 크기로 맞춘다 (Load가 1칸 볼륨을 만들 때) */
	void FitToSingleTile(float TileSize);

	UPROPERTY(EditAnywhere, Category = "Marker")
	ESpawnVolumeTeam Team = ESpawnVolumeTeam::Ally;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UBoxComponent> Box;

private:
	void RefreshBakerPreview() const;
};