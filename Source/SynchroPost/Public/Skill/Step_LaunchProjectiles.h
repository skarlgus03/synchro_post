#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillStep.h"
#include "Step_LaunchProjectiles.generated.h"

class AProjectileActor;
class UNiagaraSystem;

UCLASS(DisplayName = "투사체 발사")
class SYNCHROPOST_API UStep_LaunchProjectiles : public USkillStep
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSubclassOf<AProjectileActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftObjectPtr<UNiagaraSystem> ImpactEffect;

	/** 비어있으면 캐스터 위치에서 발사 */
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName MuzzleSocketName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 2000.f;

	/** 0이면 동시 발사. 0보다 크면 그 간격으로 순차 발사 (연사) */
	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.0"))
	float IntervalBetweenShots = 0.f;

	/** true면 마지막 투사체가 도착해야 다음 줄로. false면 쏘기만 하고 즉시 다음 줄 */
	UPROPERTY(EditAnywhere, Category = "Projectile")
	bool bWaitForAllArrivals = true;

	virtual void Start(const FSkillPresentationContext& InCtx) override;
	virtual void Abort() override;

private:
	void LaunchOne(int32 Index);
	void HandleArrival(const FCombatEventTarget& ArrivedResult);

	UPROPERTY()
	TArray<FCombatEventTarget> MyTargets;

	TArray<FTimerHandle> ShotTimers;
	int32 PendingArrivals = 0;
};