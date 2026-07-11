
#pragma once

#include "CoreMinimal.h"
#include "Types/SPGameplayTags.h"
#include "Tile.generated.h"

UENUM()
enum class ETileType : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Obstacle UMETA(DisplayName = "Obstacle"),
	SpawnPoint UMETA(DisplayName = "Spawn Point")
};

USTRUCT(BlueprintType)
struct FTileEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile", meta = (Categories = "Tile.Effect"))
	FGameplayTag EffectTag;

	// -1 means infinite duration
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	int32 RemainingDuration = -1;

	FTileEffectEntry() {}
	FTileEffectEntry(const FGameplayTag& InTag, int32 InDuration) : EffectTag(InTag), RemainingDuration(InDuration) {}
};

USTRUCT(BlueprintType)
struct FTile : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	FIntPoint Coordinate = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	ETileType TileType = ETileType::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Tile")
	TObjectPtr<AUnit> OccupyingUnit = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Tile")
	TArray<FTileEffectEntry> TileEffects;

	bool HasEffectTag(const FGameplayTag& Tag) const
	{
		return TileEffects.ContainsByPredicate([&Tag](const FTileEffectEntry& Entry) {
			return Entry.EffectTag == Tag;
			});
	}
	bool IsOccupied() const { return OccupyingUnit != nullptr;	}
	bool IsWalkable() const { return TileType != ETileType::Obstacle && !IsOccupied() && !HasEffectTag(SPTags::Tile::Effect::Wall); }
};

USTRUCT(BlueprintType)
struct FTileGrid : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTile> Tiles;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTile>(Tiles, DeltaParms, *this);
	}
	FTile* FindTile(const FIntPoint& Coordinate)
	{
		return Tiles.FindByPredicate([&Coordinate](const FTile& Tile) { return Tile.Coordinate == Coordinate; });
	}
	const FTile* FindTile(const FIntPoint& Coordinate) const
	{
		return Tiles.FindByPredicate([&Coordinate](const FTile& Tile) { return Tile.Coordinate == Coordinate; });
	}
};

template<>
struct TStructOpsTypeTraits<FTileGrid> : public TStructOpsTypeTraitsBase2<FTileGrid>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};