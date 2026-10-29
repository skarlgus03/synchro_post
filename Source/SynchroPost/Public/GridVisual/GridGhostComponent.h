#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GridGhostComponent.generated.h"

class AUnit;
class UMaterialInterface;

/**
 * 이동 목적지에 서 있을 유닛의 반투명 미리보기.
 * 유닛 메시를 리더 포즈로 따라 해서, 자기 애니메이션 없이도 같은 자세(Idle 등)를 보인다.
 * 로컬 연출 전용 (복제 안 함, 콜리전·그림자 없음)
 */
UCLASS()
class SYNCHROPOST_API UGridGhostComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UGridGhostComponent(const FObjectInitializer& ObjectInitializer);

	// Unit이 FacingFrom 칸에서 Coord 칸으로 들어와 선 모습을 보인다
	void ShowGhost(AUnit* Unit, const FIntPoint& Coord, const FIntPoint& FacingFrom);
	void HideGhost();

protected:
	// 모든 머티리얼 슬롯을 이걸로 덮는다. GridReveal(MPC)을 곱해야 격자와 함께 페이드된다
	UPROPERTY(EditAnywhere, Category = "Ghost")
	TObjectPtr<UMaterialInterface> GhostMaterial;

private:
	// 지금 따라 하는 유닛. 바뀔 때만 메시·머티리얼·리더를 다시 설정한다
	TWeakObjectPtr<AUnit> SourceUnit;
};