#include "Animation/AnimNotify_SpawnProjectile.h"
#include "Unit/Unit.h"
#include "Skill/ProjectileActor.h"
#include "Framework/GridManager.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AUnit* Unit = MeshComp ? Cast<AUnit>(MeshComp->GetOwner()) : nullptr;
	if (!Unit || !ProjectileClass)
	{
		return;
	}

	UGridManager* GridManager = Unit->GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager)
	{
		return;
	}

	const FVector SpawnLocation = SpawnSocketName != NAME_None
		? MeshComp->GetSocketLocation(SpawnSocketName)
		: Unit->GetActorLocation();

	for (const FCombatEventTarget& TargetData : Unit->GetCurrentSkillPresentationTargets())
	{
		const FVector Destination = GridManager->GetTileWorldLocation(TargetData.Coordinate);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Unit;

		if (AProjectileActor* Projectile = Unit->GetWorld()->SpawnActor<AProjectileActor>(
			ProjectileClass, SpawnLocation, (Destination - SpawnLocation).Rotation(), SpawnParams))
		{
			Projectile->Initialize(TargetData, Destination, ImpactEffect, Unit, ProjectileSpeed);
		}
	}
}