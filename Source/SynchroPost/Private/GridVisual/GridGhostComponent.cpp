#include "GridVisual/GridGhostComponent.h"

#include "Unit/Unit.h"
#include "Unit/UnitPresentationBase.h"
#include "SynchroPost.h"

UGridGhostComponent::UGridGhostComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 커서 트레이스에 걸리지 않게
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CastShadow = false;

	SetVisibility(false);
}

void UGridGhostComponent::ShowGhost(AUnit* Unit, const FIntPoint& Coord, const FIntPoint& FacingFrom)
{
	USkeletalMeshComponent* SourceMesh = Unit ? Unit->GetMesh() : nullptr;
	if (!SourceMesh || !SourceMesh->GetSkeletalMeshAsset())
	{
		HideGhost();
		return;
	}

	// 유닛이 바뀌었을 때만: 메시 교체 → 모든 슬롯을 고스트 머티리얼로 → 리더 포즈
	if (SourceUnit.Get() != Unit)
	{
		SourceUnit = Unit;
		SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());

		if (GhostMaterial)
		{
			for (int32 Slot = 0; Slot < GetNumMaterials(); ++Slot)
			{
				SetMaterial(Slot, GhostMaterial);
			}
		}
		else
		{
			UE_LOG(LogSP, Warning, TEXT("[Ghost] GhostMaterial is not set"));
		}

		SetLeaderPoseComponent(SourceMesh);
	}

	// 실제 유닛이 설 자리: (서 있을 위치, 도착 방향, 액터 스케일) × 유닛 메시의 상대 트랜스폼
	// 같은 칸이면 CalcFacingRotation이 false → 지금 방향 유지
	FRotator Facing = Unit->GetActorRotation();
	UUnitPresentationBase::CalcFacingRotation(FacingFrom, Coord, Facing);
	const FTransform ActorTransform(Facing, Unit->GetStandLocation(Coord), Unit->GetActorScale3D());
	SetWorldTransform(SourceMesh->GetRelativeTransform() * ActorTransform);

	SetVisibility(true);
}

void UGridGhostComponent::HideGhost()
{
	SetVisibility(false);

	// 유닛이 사라져도 끊긴 리더가 남지 않게 놓아준다
	SetLeaderPoseComponent(nullptr);
	SourceUnit.Reset();
}