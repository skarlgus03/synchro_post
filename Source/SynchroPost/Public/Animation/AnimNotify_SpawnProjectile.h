#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnProjectile.generated.h"

class AProjectileActor;
class UNiagaraSystem;

UCLASS()
class SYNCHROPOST_API UAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSubclassOf<AProjectileActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName SpawnSocketName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float ProjectileSpeed = 2000.f;
};