
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
	SpawnPoint UMETA(DisplayName = "Spawn Point"),
	EnemySpawnPoint UMETA(DisplayName = "Enemy Spawn Point"),
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
	TObjectPtr<AActor> OccupyingActor = nullptr;

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
	bool IsOccupied() const { return OccupyingActor != nullptr;	}
	bool IsWalkable() const { return !IsOccupied() && !HasEffectTag(SPTags::Tile::Effect::Wall); }
};

USTRUCT(BlueprintType)
struct FTileGrid : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTile> Entries;

	// 주의 : FastARraySerializer는 Entries  만 직렬화함.
	// 아래 세 필드는 복제되지 않으며, 클라이언트에서는 UGridStateComponent가 주입함.


private:
	int32 GridWidth = 0;
	int32 GridHeight = 0;
	float GridTileSize = 100.0f;

public:
	
	// 치수 주입
	void SetDimensions(int32 InWidth, int32 InHeight, float InTileSize)
	{
		GridWidth = InWidth;
		GridHeight = InHeight;
		GridTileSize = InTileSize;
	}

	bool IsInitialized() const
	{
		return GridWidth > 0 && GridHeight > 0 && GridTileSize > 0.0f;
	}

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
	// 직접 부르면 컴포넌트의 복제 프로퍼티가 갱신되지 않아 클라이언트에 반영되지 않는다. 
	// 반드시 UGridStateComponent::InitializeGrid()에서 호출해야 한다.
	void InitializeGrid(int32 Width, int32 Height, float TileSize)
	{
		SetDimensions(Width, Height, TileSize);

		Entries.Empty();
		Entries.Reserve(Width * Height);

		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
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

	// 좌표에 액터(유닛 또는 장애물)를 점유시킨다.
	bool SetOccupantAt(const FIntPoint& Coord, AActor* Occupant)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		Tile->OccupyingActor = Occupant;
		MarkItemDirty(*Tile);
		return true;
	}

	// 특정 타일에서 점유 액터를 제거한다.
	bool ClearOccupantAt(const FIntPoint& Coord)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		Tile->OccupyingActor = nullptr;
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

	// 특정 좌표에서 특정 태그의 타일 이펙트를 추가한다. 이미 존재하면 추가하지 않는다. Duration이 -1이면 무한 지속 이펙트로 간주한다.
	bool AddTileEffect(const FIntPoint& Coord, const FGameplayTag& EffectTag, int32 Duration = -1)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		if (!Tile->HasEffectTag(EffectTag))
		{
			Tile->TileEffects.Add(FTileEffectEntry(EffectTag, Duration));
			MarkItemDirty(*Tile);
		}
		return true;
	}
	// 특정 좌표에서 특정 태그의 타일 이펙트를 제거한다.
	bool RemoveTileEffect(const FIntPoint& Coord, const FGameplayTag& EffectTag)
	{
		FTile* Tile = Find(Coord);
		if (!Tile) return false;

		const int32 RemovedCount = Tile->TileEffects.RemoveAll([&EffectTag](const FTileEffectEntry& Entry) {
			return Entry.EffectTag == EffectTag;
			});
		if (RemovedCount > 0)
		{
			MarkItemDirty(*Tile);
		}
		return true;
	}

	int32 GetGridWidth() const { return GridWidth; }
	int32 GetGridHeight() const { return GridHeight; }
	float GetGridTileSize() const { return GridTileSize; }
};

template<>
struct TStructOpsTypeTraits<FTileGrid> : public TStructOpsTypeTraitsBase2<FTileGrid>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};