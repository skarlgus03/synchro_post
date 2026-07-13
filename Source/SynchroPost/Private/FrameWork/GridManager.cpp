
#include "FrameWork/GridManager.h"
#include "Grid/TileMapDataAsset.h"

void UGridManager::LoadGrid(UTileMapDataAsset* StageData)
{
    if (!StageData) return;

    TileGrid.InitializeGrid(StageData->GridWidth, StageData->GridHeight, StageData->TileSize);

    for (const FTileSpawnInfo& SpecialTile : StageData->SpecialTiles)
    {
        TileGrid.SetTileType(SpecialTile.Coordinate, SpecialTile.TileType);
    }
}

AUnit* UGridManager::GetUnitAt(const FIntPoint& Coord) const
{
    const FTile* Tile = TileGrid.Find(Coord);
    return Tile ? Tile->OccupyingUnit : nullptr;
}

bool UGridManager::IsWalkable(const FIntPoint& Coord) const
{
    const FTile* Tile = TileGrid.Find(Coord);
    return Tile && Tile->IsWalkable();
}

void UGridManager::SetUnitAt(const FIntPoint& Coord, AUnit* Unit)
{
    TileGrid.SetUnitAt(Coord, Unit);
}

void UGridManager::ClearUnitAt(const FIntPoint& Coord)
{
    TileGrid.ClearUnitAt(Coord);
}

void UGridManager::SetTileType(const FIntPoint& Coord, ETileType NewType)
{
    TileGrid.SetTileType(Coord, NewType);
}