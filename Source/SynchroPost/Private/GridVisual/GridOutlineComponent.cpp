#include "GridVisual/GridOutlineComponent.h"

#include "Framework/GridManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/GridOutline.h"
#include "SynchroPost.h"

namespace GridOutlineComponentPrivate
{
	struct FRibbonBuffers
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
	};

	// 두 꼭짓점 사이 방향 (고리의 변은 항상 축 방향이라 성분이 -1, 0, 1 중 하나)
	FIntPoint UnitDir(const FIntPoint& From, const FIntPoint& To)
	{
		return FIntPoint(FMath::Sign(To.X - From.X), FMath::Sign(To.Y - From.Y));
	}

	// 진행 방향의 왼쪽 = 영역 안쪽 (GridOutline 고리 방향 규칙)
	FVector2D LeftOf(const FIntPoint& Dir)
	{
		return FVector2D(-Dir.Y, Dir.X);
	}

	// 고리 하나 → 액자 모양 띠. 꼭짓점마다 안쪽으로 Inset, Inset + BandWidth 만큼 민 점 두 개
	void AppendLoopRibbon(const TArray<FIntPoint>& Loop, const UGridManager& Grid, const FTransform& ComponentTransform,
		const FGridOutlineStyle& Style, FRibbonBuffers& Out)
	{
		const int32 N = Loop.Num();
		if (N < 3)
		{
			return;
		}

		const int32 Base = Out.Vertices.Num();
		float U = 0.0f;

		// 마지막에 첫 꼭짓점을 한 번 더 넣는다 → U가 0에서 둘레 길이까지 끊기지 않고 이어짐 (흐르는 효과용)
		for (int32 Step = 0; Step <= N; ++Step)
		{
			const int32 Index = Step % N;
			const FIntPoint& Prev = Loop[(Index + N - 1) % N];
			const FIntPoint& Cur = Loop[Index];
			const FIntPoint& Next = Loop[(Index + 1) % N];

			// 들어오는 변과 나가는 변의 안쪽 방향을 더한다.
			// 고리는 항상 직각으로 꺾이므로 볼록·오목 모서리 모두 두 변에서 정확히 같은 거리만큼 들어간 점이 된다
			const FVector2D Inward = LeftOf(UnitDir(Prev, Cur)) + LeftOf(UnitDir(Cur, Next));
			const FVector Offset(Inward.X, Inward.Y, 0.0);

			const FVector Corner = GridOutline::CornerToWorld(Grid, Cur) + FVector(0.0, 0.0, Style.FloatHeight);
			const FVector OuterWorld = Corner + Offset * Style.Inset;
			const FVector InnerWorld = Corner + Offset * (Style.Inset + Style.BandWidth);

			// 메시 좌표는 컴포넌트 기준
			Out.Vertices.Add(ComponentTransform.InverseTransformPosition(OuterWorld));
			Out.Vertices.Add(ComponentTransform.InverseTransformPosition(InnerWorld));
			Out.Normals.Add(FVector::UpVector);
			Out.Normals.Add(FVector::UpVector);
			Out.UVs.Add(FVector2D(U, 0.0f));   // V 0 = 바깥 가장자리
			Out.UVs.Add(FVector2D(U, 1.0f));   // V 1 = 안쪽 가장자리

			if (Step < N)
			{
				U += FMath::Abs(Next.X - Cur.X) + FMath::Abs(Next.Y - Cur.Y);   // 칸 단위 길이
			}
		}

		// 이웃한 두 꼭짓점의 점 4개 = 사각형 = 삼각형 2개 (머티리얼이 양면이라 감는 방향은 상관없음)
		for (int32 Step = 0; Step < N; ++Step)
		{
			const int32 Outer0 = Base + Step * 2;
			const int32 Inner0 = Outer0 + 1;
			const int32 Outer1 = Outer0 + 2;
			const int32 Inner1 = Outer0 + 3;
			Out.Triangles.Append({ Outer0, Outer1, Inner0, Inner0, Outer1, Inner1 });
		}
	}
}

UGridOutlineComponent::UGridOutlineComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 커서 트레이스에 걸리지 않게
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CastShadow = false;

	// 바닥 격자보다 항상 위에 그려지게
	TranslucencySortPriority = 10;

	// 층별 기본 스타일 (BP에서 조정)
	FGridOutlineStyle Range;
	Range.Color = FLinearColor(0.2f, 0.6f, 1.0f);
	Range.FloatHeight = 3.0f;
	Styles.Add(EGridOutlineLayer::Range, Range);

	FGridOutlineStyle Impact;
	Impact.Color = FLinearColor(1.0f, 0.35f, 0.2f);
	Impact.FloatHeight = 4.0f;
	Styles.Add(EGridOutlineLayer::Impact, Impact);

	FGridOutlineStyle Target;
	Target.Color = FLinearColor::White;
	Target.FloatHeight = 5.0f;
	Styles.Add(EGridOutlineLayer::Target, Target);
}

void UGridOutlineComponent::SetLayer(EGridOutlineLayer Layer, const TArray<FIntPoint>& Tiles)
{
	using namespace GridOutlineComponentPrivate;
	
	const int32 Section = static_cast<int32>(Layer);

	const UGridManager* Grid = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (Tiles.Num() == 0 || !Grid || Grid->GetTileSize() <= 0.f)
	{
		UE_LOG(LogSP, Warning, TEXT("[GridOutline] SetLayer : Empty tiles or invalid grid"));
		ClearMeshSection(Section);
		return;
	}

	const FGridOutlineStyle& Style = GetStyle(Layer);

	// Inset + BandWith는 타일 절반보다 작아야 한다. (그 이상이면 띠가 겹쳐서 안됨)
	if (Style.Inset + Style.BandWidth > Grid->GetTileSize() * 0.5f)
	{
		UE_LOG(LogSP, Warning, TEXT("[GridOutline] Inset(%.1f) + BandWidth(%.1f) must be less than half a tile (%.1f)"),
			Style.Inset, Style.BandWidth, Grid->GetTileSize() * 0.5f);
		ClearMeshSection(Section);
		return;
	}

	// 버퍼 채우기
	FRibbonBuffers Buffers;
	for (const TArray<FIntPoint>& Loop : GridOutline::BuildLoops(Tiles))
	{
		AppendLoopRibbon(Loop, *Grid, GetComponentTransform(), Style, Buffers);
	}

	// 버퍼로 메시 만듬
	CreateMeshSection(Section, Buffers.Vertices, Buffers.Triangles, Buffers.Normals, Buffers.UVs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision*/ false);

	if (UMaterialInstanceDynamic* Material = GetLayerMaterial(Layer))
	{
		SetMaterial(Section, Material);
	}
}

void UGridOutlineComponent::ClearLayer(EGridOutlineLayer Layer)
{
	ClearMeshSection(static_cast<int32>(Layer));
}

void UGridOutlineComponent::ClearAllLayers()
{
	ClearAllMeshSections();
}

const FGridOutlineStyle& UGridOutlineComponent::GetStyle(EGridOutlineLayer Layer) const
{
	static const FGridOutlineStyle Default;
	const FGridOutlineStyle* Found = Styles.Find(Layer);
	return Found ? *Found : Default;
}

UMaterialInstanceDynamic* UGridOutlineComponent::GetLayerMaterial(EGridOutlineLayer Layer)
{
	if (!OutlineMaterial)
	{
		UE_LOG(LogSP, Warning, TEXT("[GridOutline] OutlineMaterial is not set"));
		return nullptr;
	}

	// 층별 머티리얼 인스턴스 배열을 충분히 늘린다
	const int32 Index = static_cast<int32>(Layer);
	if (LayerMaterials.Num() <= Index)
	{
		LayerMaterials.SetNum(static_cast<int32>(EGridOutlineLayer::Count));
	}

	if (!LayerMaterials[Index])
	{
		const FGridOutlineStyle& Style = GetStyle(Layer);
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), Style.Color);
		Material->SetScalarParameterValue(TEXT("Intensity"), Style.Intensity);
		LayerMaterials[Index] = Material;
	}
	return LayerMaterials[Index];
}
