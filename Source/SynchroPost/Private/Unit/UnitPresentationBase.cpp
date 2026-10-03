#include "Unit/UnitPresentationBase.h"
#include "Unit/Unit.h"
#include "Math/GridMath.h"

void UUnitPresentationBase::PresentDeath_Implementation(AUnit* Owner)
{
	if (!Owner) { return; }

	Owner->SetActorHiddenInGame(true);
	Owner->SetActorEnableCollision(false);
	Owner->NotifyMyPresentationFinished();
}

void UUnitPresentationBase::PresentRevive_Implementation(AUnit* Owner)
{
	if (!Owner) { return; }

	Owner->SetActorHiddenInGame(false);
	Owner->SetActorEnableCollision(true);
	Owner->NotifyMyPresentationFinished();
}

void UUnitPresentationBase::PresentMoveSegment_Implementation(AUnit* Owner, const TArray<FIntPoint>& Waypoints)
{
	if (!Owner)
	{
		return;
	}
	if (Waypoints.Num() > 0)
	{
		Owner->SnapToTile(Waypoints.Last());
	}
	Owner->NotifyMyPresentationFinished();
}

void UUnitPresentationBase::PresentHit_Implementation(AUnit* Owner)
{
	
}

void UUnitPresentationBase::PresentFace_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& Toward)
{
	FRotator Target;
	if (Owner && CalcFacingRotation(From, Toward, Target))
	{
		Owner->SetActorRotation(Target);
	}
}

bool UUnitPresentationBase::CalcFacingRotation(const FIntPoint& From, const FIntPoint& Toward, FRotator& OutRotation)
{
	if (From == Toward)
	{
		return false;
	}

	const FIntPoint Dir = GridMath::ToCardinalDirection(Toward - From);
	OutRotation = FRotator(0.f, FVector(Dir.X, Dir.Y, 0.f).Rotation().Yaw, 0.f);
	return true;
}
