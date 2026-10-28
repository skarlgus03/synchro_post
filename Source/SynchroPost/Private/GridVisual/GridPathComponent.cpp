#include "GridVisual/GridPathComponent.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "SynchroPost.h"

namespace GridPathComponentPrivate
{
	struct FArrowBuffers
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		
		void Add(const FTransform& ComponentTransform, const FVector& World, const FVector2D& UV)
		{
			Vertices.Add(ComponentTransform.InverseTransformPosition(World)); // 메시 좌표는 컴포넌트 기준
			Normals.Add(FVector::UpVector);
			UVs.Add(UV);
		}
	};

	// 진행 방향의 왼쪽 (수평)
	FVector LeftOf(const FVector& Dir)
	{
		return FVector(-Dir.Y, Dir.X, 0.0);
	}

	// 출발점에서 Distance만큼 간 꺾은선 위의 점
	FVector PointAtDistance(const TArray<FVector>& Points, const TArray<float>& CumDist, float Distance)
	{
		int32 Seg = 1;
		while (Seg < Points.Num() - 1 && CumDist[Seg] < Distance)
		{
			++Seg;
		}
		const float SegLen = CumDist[Seg] - CumDist[Seg - 1];
		const float T = (SegLen > KINDA_SMALL_NUMBER) ? (Distance - CumDist[Seg - 1]) / SegLen : 0.0f;
		return FMath::Lerp(Points[Seg - 1], Points[Seg], T);
	}

	// 꺾은선의 [From, To] 거리 구간만 잘라낸다. 경계 근처 점은 버려서 길이 0인 선분을 만들지 않는다
	TArray<FVector> SubPolyline(const TArray<FVector>& Points, const TArray<float>& CumDist, float From, float To)
	{
		constexpr float MinGap = 0.5f;

		TArray<FVector> Out;
		Out.Add(PointAtDistance(Points, CumDist, From));
		for (int32 i = 1; i < Points.Num() - 1; ++i)
		{
			if (CumDist[i] > From + MinGap && CumDist[i] < To - MinGap)
			{
				Out.Add(Points[i]);
			}
		}
		Out.Add(PointAtDistance(Points, CumDist, To));
		return Out;
	}

	// 몸통: 점마다 좌우로 벌린 점 두 개 → 이웃한 두 쌍이 사각형
	void AppendBody(const TArray<FVector>& Points, float StartU, float HalfWidth, float UVScale,
		const FTransform& ComponentTransform, FArrowBuffers& Out)
	{
		const int32 N = Points.Num();
		const int32 Base = Out.Vertices.Num();
		float U = StartU;

		for (int32 i = 0; i < N; ++i)
		{
			const FVector DirIn = (i > 0) ? (Points[i] - Points[i - 1]).GetSafeNormal2D() : FVector::ZeroVector;
			const FVector DirOut = (i < N - 1) ? (Points[i + 1] - Points[i]).GetSafeNormal2D() : FVector::ZeroVector;

			// 양 끝은 한쪽 선분, 중간은 두 선분의 평균 방향
			const FVector Side = LeftOf((DirIn + DirOut).GetSafeNormal2D());

			// 꺾인 곳에서 폭이 얇아지지 않게 1/cos 보정 (최대 2배)
			const FVector RefSide = LeftOf(DirIn.IsZero() ? DirOut : DirIn);
			const float Miter = 1.0f / FMath::Max(static_cast<float>(FVector::DotProduct(Side, RefSide)), 0.5f);

			if (i > 0)
			{
				U += static_cast<float>(FVector::Dist(Points[i - 1], Points[i])) / UVScale;
			}

			const FVector Offset = Side * (HalfWidth * Miter);
			Out.Add(ComponentTransform, Points[i] + Offset, FVector2D(U, 0.0f)); // 왼쪽
			Out.Add(ComponentTransform, Points[i] - Offset, FVector2D(U, 1.0f)); // 오른쪽
		}

		// 머티리얼이 양면이라 감는 방향은 상관없음
		for (int32 i = 0; i < N - 1; ++i)
		{
			const int32 L0 = Base + i * 2;
			const int32 R0 = L0 + 1;
			const int32 L1 = L0 + 2;
			const int32 R1 = L0 + 3;
			Out.Triangles.Append({ L0, L1, R0, R0, L1, R1 });
		}
	}

	// 화살촉: 밑변 양끝 + 끝점
	void AppendHead(const FVector& BaseCenter, const FVector& Tip, float HalfWidth, float BaseU, float TipU,
		const FTransform& ComponentTransform, FArrowBuffers& Out)
	{
		const FVector Side = LeftOf((Tip - BaseCenter).GetSafeNormal2D()) * HalfWidth;
		const int32 Base = Out.Vertices.Num();

		Out.Add(ComponentTransform, BaseCenter + Side, FVector2D(BaseU, 0.0f));
		Out.Add(ComponentTransform, BaseCenter - Side, FVector2D(BaseU, 1.0f));
		Out.Add(ComponentTransform, Tip, FVector2D(TipU, 0.5f));
		Out.Triangles.Append({ Base, Base + 1, Base + 2 });
	}
}

UGridPathComponent::UGridPathComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 복제 안 함, 콜리전 없음, 그림자 안 드리움
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CastShadow = false;

	// 외곽선 보다 위에
	TranslucencySortPriority = 11;
}	

void UGridPathComponent::SetPath(const TArray<FVector>& WorldPoints)
{
	using namespace GridPathComponentPrivate;

	// 점이 2개 미만이면 지움
	if (WorldPoints.Num() < 2)
	{
		ClearPath();
		return;
	}

	// 띄운 점 + 누적 거리
	TArray<FVector> Points;
	TArray<float> CumDist;
	Points.Reserve(WorldPoints.Num());
	CumDist.Reserve(WorldPoints.Num());
	for (const FVector& P : WorldPoints)
	{
		Points.Add(P + FVector(0.0, 0.0, FloatHeight));
		CumDist.Add(CumDist.Num() == 0 ? 0.0f
			: CumDist.Last() + static_cast<float>(FVector::Dist(Points[Points.Num() - 2], Points.Last())));
	}

	// 총 거리 계산 길이 0이면 지움
	const float Total = CumDist.Last();
	if (Total <= KINDA_SMALL_NUMBER)
	{
		ClearPath();
		return;
	}

	// 짧으면 StartGap부터 줄이고, 그래도 모자라면 화살촉만
	const float Head = FMath::Min(HeadLength, Total);
	const float BodyTo = Total - Head;
	const float BodyFrom = FMath::Min(StartGap, BodyTo);

	// 메시 만들기
	FArrowBuffers Buffers;
	const FTransform& ComponentTransform = GetComponentTransform();

	// 몸통이 1cm 이상이면 만들고, 아니면 화살촉만
	if (BodyTo - BodyFrom > 1.0f)
	{
		AppendBody(SubPolyline(Points, CumDist, BodyFrom, BodyTo), BodyFrom / UVScale, Width * 0.5f, UVScale,
			ComponentTransform, Buffers);
	}
	AppendHead(PointAtDistance(Points, CumDist, BodyTo), Points.Last(), HeadWidth * 0.5f,
		BodyTo / UVScale, Total / UVScale, ComponentTransform, Buffers);

	// 메시 섹션 생성. 콜리전은 안 만듦
	CreateMeshSection(0, Buffers.Vertices, Buffers.Triangles, Buffers.Normals, Buffers.UVs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision*/ false);

	if (PathMaterial)
	{
		SetMaterial(0, PathMaterial);
	}
}

void UGridPathComponent::ClearPath()
{
	ClearAllMeshSections();
}

