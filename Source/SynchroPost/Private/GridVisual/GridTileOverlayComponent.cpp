#include "GridVisual/GridTileOverlayComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Framework/GridManager.h"
#include "SynchroPost.h"

UGridTileOverlayComponent::UGridTileOverlayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridTileOverlayComponent::SetLayer(const FGameplayTag& Tag, const TArray<FIntPoint>& Tiles)
{
	if (Tiles.Num() == 0)
	{
		ClearLayer(Tag);
		return;
	}

	const UGridManager* Grid = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!Grid || Grid->GetTileSize() <= 0.f)
	{
		UE_LOG(LogSP, Warning, TEXT("[TileOverlay] SetLayer %s : invalid grid"), *Tag.ToString());
		return;
	}

	UInstancedStaticMeshComponent* Layer = GetOrCreateLayer(Tag);
	if (!Layer)
	{
		return;
	}

	// GetOrCreateLayer가 스타일과 메시를 확인했다
	const FTileOverlayStyle& Style = Styles.FindChecked(Tag);
	const FVector MeshSize = Style.Mesh->GetBounds().BoxExtent * 2.0;
	if (MeshSize.X <= KINDA_SMALL_NUMBER || MeshSize.Y <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogSP, Warning, TEXT("[TileOverlay] %s : 메시 XY 크기가 0"), *Tag.ToString());
		return;
	}

	// XY만 칸에 맞추고 Z는 메시 그대로 (상자 메시의 높이 유지)
	const double Size = Grid->GetTileSize() * Style.TileScale;
	const FVector Scale(Size / MeshSize.X, Size / MeshSize.Y, 1.0);

	TArray<FTransform> Transforms;
	Transforms.Reserve(Tiles.Num());
	for (const FIntPoint& Tile : Tiles)
	{
		const FVector Location = Grid->GetTileWorldLocation(Tile) + FVector(0.0, 0.0, Style.Height);
		Transforms.Emplace(FRotator::ZeroRotator, Location, Scale);
	}

	Layer->ClearInstances();
	Layer->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
}

void UGridTileOverlayComponent::ClearLayer(const FGameplayTag& Tag)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Layers.Find(Tag))
	{
		(*Found)->ClearInstances();
	}
}

void UGridTileOverlayComponent::ClearAllLayers()
{
	for (TPair<FGameplayTag, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : Layers)
	{
		Pair.Value->ClearInstances();
	}
}

UInstancedStaticMeshComponent* UGridTileOverlayComponent::GetOrCreateLayer(const FGameplayTag& Tag)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Layers.Find(Tag))
	{
		return *Found;
	}

	const FTileOverlayStyle* Style = Styles.Find(Tag);
	if (!Style || !Style->Mesh)
	{
		UE_LOG(LogSP, Warning, TEXT("[TileOverlay] %s : 스타일 또는 메시가 없음"), *Tag.ToString());
		return nullptr;
	}

	UInstancedStaticMeshComponent* Layer = NewObject<UInstancedStaticMeshComponent>(GetOwner());
	Layer->SetStaticMesh(Style->Mesh);
	Layer->SetMaterial(0, Style->Material);
	Layer->TranslucencySortPriority = Style->SortPriority;

	// 커서 트레이스에 걸리지 않게
	Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Layer->SetGenerateOverlapEvents(false);
	Layer->SetCanEverAffectNavigation(false);
	Layer->SetCastShadow(false);

	Layer->SetupAttachment(this);
	Layer->RegisterComponent();

	Layers.Add(Tag, Layer);
	return Layer;
}