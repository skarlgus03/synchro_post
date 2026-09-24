#include "Unit/UnitAttachment.h"
#include "Unit/Unit.h"
#include "SynchroPost.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

UMeshComponent* UUnitAttachment::CreateAttachedComponent(AUnit* Owner) const
{
	USkeletalMeshComponent* OwnerMesh = Owner ? Owner->GetMesh() : nullptr;
	if (!OwnerMesh)
	{
		UE_LOG(LogSP, Error, TEXT("[Attachment] %s: 주인 유닛 또는 메시 없음"), *GetNameSafe(this));
		return nullptr;
	}
	UMeshComponent* NewComp = NewMeshComponent(Owner);
	if (!NewComp)
	{
		return nullptr;   // 사유는 NewMeshComponent가 이미 로그로 남김
	}

	if (!AttachSocketName.IsNone() && !OwnerMesh->DoesSocketExist(AttachSocketName))
	{
		UE_LOG(LogSP, Warning, TEXT("[Attachment] %s: 소켓 '%s'이(가) %s에 없음 — 메시 원점에 부착됨"),
			*GetNameSafe(Owner), *AttachSocketName.ToString(), *GetNameSafe(OwnerMesh->GetSkeletalMeshAsset()));
	}

	NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComp->SetGenerateOverlapEvents(false);
	NewComp->SetupAttachment(OwnerMesh, AttachSocketName);   // 등록 전이므로 SetupAttachment
	NewComp->SetRelativeTransform(RelativeTransform);
	NewComp->RegisterComponent();
	return NewComp;
}

UMeshComponent* UUnitAttachment::NewMeshComponent(AUnit* Owner) const
{
	// 베이스는 Abstract라 DA에 올라올 수 없지만, 서브클래스가 override를 빠뜨린 경우를 잡는다
	UE_LOG(LogSP, Error, TEXT("[Attachment] %s: NewMeshComponent 미구현"), *GetClass()->GetName());
	return nullptr;
}

UMeshComponent* USkeletalMeshAttachment::NewMeshComponent(AUnit* Owner) const
{
	USkeletalMesh* Mesh = SkeletalMesh.LoadSynchronous();
	if (!Mesh)
	{
		UE_LOG(LogSP, Error, TEXT("[Attachment] %s: SkeletalMesh가 비어 있거나 로드 실패"), *GetNameSafe(Owner));
		return nullptr;
	}
	USkeletalMeshComponent* NewComp = NewObject<USkeletalMeshComponent>(Owner);
	NewComp->SetSkeletalMesh(Mesh);
	return NewComp;
}

UMeshComponent* UStaticMeshAttachment::NewMeshComponent(AUnit* Owner) const
{
	UStaticMesh* Mesh = StaticMesh.LoadSynchronous();
	if (!Mesh)
	{
		UE_LOG(LogSP, Error, TEXT("[Attachment] %s: StaticMesh가 비어 있거나 로드 실패"), *GetNameSafe(Owner));
		return nullptr;
	}
	UStaticMeshComponent* NewComp = NewObject<UStaticMeshComponent>(Owner);
	NewComp->SetStaticMesh(Mesh);
	return NewComp;
}