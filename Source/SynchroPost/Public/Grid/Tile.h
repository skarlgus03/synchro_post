
#pragma once

#include "CoreMinimal.h"
#include "Types/SPGameplayTags.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Tile.generated.h"

class AUnit;

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
	TArray<FTile> Entries;

	// 그리드의 가로 길이. 이 값은 그리드 초기화 시 설정된다.
	UPROPERTY()
	int32 GridWidth = 0;

	// 그리드의 세로 길이. 이 값은 그리드 초기화 시 설정된다.
	UPROPERTY()
	int32 GridHeight = 0;


	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTile>(Entries, DeltaParms, *this);
	}
	
	// 좌표가 올바른 지 확인하는 함수. 그리드의 범위를 벗어나면 false를 반환한다.
	bool IsValidCoord(const FIntPoint& Coord) const
	{
		return Coord.X >= 0 && Coord.X < GridWidth && Coord.Y >= 0 && Coord.Y < GridHeight;
	}

	// 좌표를 인덱스로 변환하는 함수. 그리드의 가로 길이를 고려하여 계산한다.
	int32 CoordToIndex(const FIntPoint& Coord) const
	{
		return Coord.Y * GridWidth + Coord.X;
	}

	// == 조회 ==

	FTile* Find(const FIntPoint& Coord)
	{
		if (!IsValidCoord(Coord))
		{
			return nullptr;
		}
		return &Entries[CoordToIndex(Coord)];
	}
	
	const FTile* Find(const FIntPoint& Coord) const
	{
		if (!IsValidCoord(Coord))
		{
			return nullptr;
		}
		return &Entries[CoordToIndex(Coord)];
	}

	// 그리드를 초기화한다. 모든 타일을 Normal타일로 초기화한다.
	void InitializeGrid(int32 Width, int32 Height, float TileSize)
	{
		GridWidth = Width;
		GridHeight = Height;

		Entries.Empty();
		Entries.Reserve(Width * Height);

		for (int32 X = 0; X < Width; ++X)
		{
			for (int32 Y = 0; Y < Height; ++Y)
			{
				FTile NewTile;
				NewTile.Coordinate = FIntPoint(X, Y);
				NewTile.TileType = ETileType::Normal;
				NewTile.WorldLocation = FVector(X * TileSize, Y * TileSize, 0.0f);
				Entries.Add(NewTile);
			}
		}

		MarkArrayDirty();
	}

	// 유닛을 특정 타일에 배치한다. 성공하면 true, 실패하면 false를 반환한다.
	bool SetUnitAt(const FIntPoint& Coord, AUnit* Unit)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		Tile->OccupyingUnit = Unit;
		MarkItemDirty(*Tile);
		return true;
	}

	// 특정 타일에서 유닛을 제거한다. 성공하면 true, 실패하면 false를 반환한다.
	bool ClearUnitAt(const FIntPoint& Coord)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		Tile->OccupyingUnit = nullptr;
		MarkItemDirty(*Tile);
		return true;
	}
	
	// 특정 타일의 타입을 변경한다. 성공하면 true, 실패하면 false를 반환한다.
	bool SetTileType(const FIntPoint& Coord, ETileType NewType)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		Tile->TileType = NewType;
		MarkItemDirty(*Tile);
		return true;
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