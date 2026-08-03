#include "Framework/GridVisualizer.h"
#include "Framework/GridManager.h"
#include "Components/InstancedStaticMeshComponent.h"


AGridVisualizer::AGridVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	GridMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridMesh"));
	RootComponent = GridMesh;
	GridMesh->NumCustomDataFloats = 4; // For visual state (RGBA)
}


void AGridVisualizer::PopulateFromGrid()
{
	if (!CachedGridManager)
	{
		CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	}
	if (!CachedGridManager)
	{
		return;
	}

	GridMesh->ClearInstances();
	CoordToInstanceIndex.Empty();

	for (const FTile& Tile : CachedGridManager->GetAllTiles())
	{
		const FVector InstanceLocation = Tile.WorldLocation + FVector(0.f, 0.f, HeightOffset);
		const FTransform InstanceTransform(FRotator::ZeroRotator, InstanceLocation, FVector(TileScale, TileScale, 1.0f));

		// 인스턴스 추가 후, 인스턴스의 인덱스를 저장하고, 초기 상태를 설정합니다.
		const int32 InstanceIndex = GridMesh->AddInstance(InstanceTransform, true);

		FLinearColor DefaultColor = ResolveColor(ETileVisualState::Default);
		GridMesh->SetCustomDataValue(InstanceIndex, 0, DefaultColor.R);
		GridMesh->SetCustomDataValue(InstanceIndex, 1, DefaultColor.G);
		GridMesh->SetCustomDataValue(InstanceIndex, 2, DefaultColor.B);
		GridMesh->SetCustomDataValue(InstanceIndex, 3, DefaultColor.A);

		CoordToInstanceIndex.Add(Tile.Coordinate, InstanceIndex);
	}
}

void AGridVisualizer::AddTileState(const FIntPoint& Coord, ETileVisualState State)
{
	ActiveTileStates.FindOrAdd(Coord).States.AddUnique(State);
	RefreshTileVisual(Coord);
}

void AGridVisualizer::RemoveTileState(const FIntPoint& Coord, ETileVisualState State)
{
	if (FTileStateList* Entry = ActiveTileStates.Find(Coord))
	{
		Entry->States.Remove(State);
		RefreshTileVisual(Coord);
	}
}

void AGridVisualizer::AddTileStates(const TArray<FIntPoint>& Coords, ETileVisualState State)
{
	for (const FIntPoint& Coord : Coords)
	{
		AddTileState(Coord, State);
	}
}

void AGridVisualizer::RemoveTileStates(const TArray<FIntPoint>& Coords, ETileVisualState State)
{
	for (const FIntPoint& Coord : Coords)
	{
		RemoveTileState(Coord, State);
	}
}

void AGridVisualizer::RefreshTileVisual(const FIntPoint& Coord)
{
	const int32* Index = CoordToInstanceIndex.Find(Coord);
	if (!Index)
	{
		return;
	}

	ETileVisualState Highest = ETileVisualState::Default;

	if (const FTileStateList* Entry = ActiveTileStates.Find(Coord))
	{
		for (ETileVisualState State : Entry->States)
		{
			if (static_cast<uint8>(State) > static_cast<uint8>(Highest))
			{
				Highest = State;
			}
		}
	}

	const FLinearColor Color = ResolveColor(Highest);
	GridMesh->SetCustomDataValue(*Index, 0, Color.R);
	GridMesh->SetCustomDataValue(*Index, 1, Color.G);
	GridMesh->SetCustomDataValue(*Index, 2, Color.B);
	GridMesh->SetCustomDataValue(*Index, 3, Color.A);
}

FLinearColor AGridVisualizer::ResolveColor(ETileVisualState State) const
{
	if (const FLinearColor* Color = StateColors.Find(State))
	{
		return *Color;
	}
	return FLinearColor::White;
}





