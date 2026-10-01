#include "Unit/UnitPresentationBase.h"
#include "Unit/Unit.h"
#include "Framework/GridManager.h"
#include "Math/GridMath.h"

void UUnitPresentationBase::PresentDeath_Implementation(AUnit* Owner)
{
	if (Owner)
	{
		Owner->NotifyMyPresentationFinished();
	}
}

void UUnitPresentationBase::PresentRevive_Implementation(AUnit* Owner)
{
	if (Owner)
	{
		Owner->NotifyMyPresentationFinished();
	}
}

void UUnitPresentationBase::PresentMoveSegment_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& To)
{
	if (!Owner)
	{
		return;
	}

	if (UGridManager* GridManager = Owner->GetWorld()->GetSubsystem<UGridManager>())
	{
		Owner->SnapToTile(To);
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
