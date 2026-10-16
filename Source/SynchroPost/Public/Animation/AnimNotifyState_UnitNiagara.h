#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "AnimNotifyState_UnitNiagara.generated.h"

/**
 * 엔진 Timed Niagara Effect와 같지만, 소켓을 몸뿐 아니라 부착물(무기 등)에서도 찾아 거기에 붙인다.
 * 바꾸는 것: SpawnEffect(붙일 곳), NotifyEnd(찾을 곳 — 엔진은 몸 메시의 직계 자식만 본다).
 * 태그 붙이기, 필드, 디테일 패널은 엔진 것 그대로.
 */
UCLASS(meta = (DisplayName = "Unit Niagara (부착물 소켓)"))
class SYNCHROPOST_API UAnimNotifyState_UnitNiagara : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()

public:
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	virtual UFXSystemComponent* SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const override;

private:
	// 붙일 곳: 유닛이면 소켓을 가진 컴포넌트(부착물 포함), 아니면(에디터 미리보기) 몸 메시
	USceneComponent* ResolveAttachParent(USkeletalMeshComponent* MeshComp, bool bWarnIfMissing) const;

	// 이 노티파이가 스폰한 이펙트. 붙였던 곳의 자식 → 없으면 액터 전체
	UFXSystemComponent* FindSpawnedEffect(USkeletalMeshComponent* MeshComp) const;
};