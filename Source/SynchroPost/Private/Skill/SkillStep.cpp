#include "Skill/SkillStep.h"
#include "Unit/Unit.h"
#include "Interface/Damageable.h"
#include "Framework/GridManager.h"
#include "GameFramework\Character.h"
#include "Components/SkeletalMeshComponent.h"

void USkillStep::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;
	Finish();
}

void USkillStep::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	OnStepFinished.ExecuteIfBound();
}

UWorld* USkillStep::GetContextWorld() const
{
	const AUnit* Caster = Ctx.Caster.Get();
	return Caster ? Caster->GetWorld() : nullptr;
}

void USkillStep::PresentTargetResult(const FCombatEventTarget& TargetData) const
{
	AActor* TargetActor = TargetData.Target.Get();
	if (!TargetActor || !TargetActor->Implements<UDamageable>())
	{
		return;
	}

	const int32 DisplayAmount = TargetData.HealthAfterChange - TargetData.HealthBeforeChange;
	UE_LOG(LogTemp, Warning, TEXT("[SP] 결과표시 %s : HP %d -> %d (%d)"),
		*GetNameSafe(TargetActor),
		TargetData.HealthBeforeChange, TargetData.HealthAfterChange, DisplayAmount);

	IDamageable::Execute_ApplyVisualDamage(
		TargetActor,
		DisplayAmount,
		TargetData.HealthAfterChange,
		TargetData.ActionData.bIsCriticalHit,
		TargetData.ActionData.ActionTypeTags);

}

FVector USkillStep::ResolveTargetLocation(const FCombatEventTarget& TargetData, FName InSocketName, const FVector& InOffset) const
{
	FVector Base = FVector::ZeroVector;

	if (const AActor* TargetActor = TargetData.Target.Get())
	{
		Base = TargetActor->GetActorLocation();

		if (!InSocketName.IsNone())
		{
			if (const ACharacter* AsChar = Cast<ACharacter>(TargetActor))
			{
				if (const USkeletalMeshComponent* Mesh = AsChar->GetMesh())
				{
					if (Mesh->DoesSocketExist(InSocketName))
					{
						Base = Mesh->GetSocketLocation(InSocketName);
					}
				}
			}
		}
	}
	else if (UWorld* World = GetContextWorld())
	{
		if (UGridManager* GridManager = World->GetSubsystem<UGridManager>())
		{
			Base = GridManager->GetTileWorldLocation(TargetData.Coordinate);
		}
	}

	return Base + InOffset;
}
