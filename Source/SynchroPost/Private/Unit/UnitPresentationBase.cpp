#include "Unit/UnitPresentationBase.h"
#include "Unit/Unit.h"
#include "Framework/GridManager.h"

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
		Owner->SetActorLocation(GridManager->GetTileWorldLocation(To));
	}

	Owner->NotifyMyPresentationFinished();
}
