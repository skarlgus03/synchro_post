#include "Animation/AnimNotifyState_UnitNiagara.h"
#include "Unit/Unit.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SynchroPost.h"

UFXSystemComponent* UAnimNotifyState_UnitNiagara::SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const
{
	if (!Template || !MeshComp)
	{
		return nullptr;
	}

	// 엔진과 같은 파라미터, 붙일 곳만 다르다
	FFXSystemSpawnParameters SpawnParams;
	SpawnParams.SystemTemplate = Template;
	SpawnParams.AttachToComponent = ResolveAttachParent(MeshComp, /*bWarnIfMissing*/ true);
	SpawnParams.AttachPointName = SocketName;
	SpawnParams.Location = LocationOffset;
	SpawnParams.Rotation = RotationOffset;
	SpawnParams.Scale = Scale;
	SpawnParams.LocationType = EAttachLocation::KeepRelativeOffset;
	SpawnParams.bAutoDestroy = !bDestroyAtEnd;

	if (UNiagaraComponent* NewComponent = UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams))
	{
		if (bApplyRateScaleAsTimeDilation && Animation)
		{
			NewComponent->SetCustomTimeDilation(Animation->RateScale);
		}
		return NewComponent;   // 태그는 부모 NotifyBegin이 붙인다
	}
	return nullptr;
}

void UAnimNotifyState_UnitNiagara::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (UFXSystemComponent* FXComponent = FindSpawnedEffect(MeshComp))
	{
		FXComponent->ComponentTags.Remove(GetSpawnedComponentTag());

		if (bDestroyAtEnd)
		{
			FXComponent->DestroyComponent();
		}
		else
		{
			FXComponent->Deactivate();   // bAutoDestroy라 남은 파티클이 사라지면 스스로 삭제
		}
	}

	// 태그를 뗐으니 부모는 아무것도 못 찾고 UAnimNotifyState::NotifyEnd만 부른다
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

USceneComponent* UAnimNotifyState_UnitNiagara::ResolveAttachParent(USkeletalMeshComponent* MeshComp, bool bWarnIfMissing) const
{
	const AUnit* Unit = Cast<AUnit>(MeshComp->GetOwner());
	if (Unit && !SocketName.IsNone())
	{
		if (UMeshComponent* SocketComp = Unit->FindSocketComponent(SocketName))
		{
			return SocketComp;
		}
		if (bWarnIfMissing)
		{
			UE_LOG(LogSP, Warning, TEXT("[UnitNiagara] %s: 소켓 '%s' 없음 → 몸 메시에 붙임"),
				*Unit->GetName(), *SocketName.ToString());
		}
		return MeshComp;   // 에디터 미리보기(유닛 아님) 또는 소켓 없음
	}

	// 에디터 미리보기 용 
	// 에디터 미리보기 : 몸 메시 -> 거기 붙은 프리뷰 부착물 순서로 소켓 찾기.
	if (!SocketName.IsNone())
	{
		TArray<USceneComponent*> Children;
		MeshComp->GetChildrenComponents(/*bIncludeAllDescendants*/ true, Children);
		for (USceneComponent* Child : Children)
		{
			if (Child && Child->DoesSocketExist(SocketName))
			{
				return Child;
			}
		}
	}
	return MeshComp;
}

UFXSystemComponent* UAnimNotifyState_UnitNiagara::FindSpawnedEffect(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}
	const FName Tag = GetSpawnedComponentTag();

	// 1) 붙였던 곳의 직계 자식 (엔진과 같은 방식. 무기에 붙였으면 무기 아래)
	if (USceneComponent* Parent = ResolveAttachParent(MeshComp, /*bWarnIfMissing*/ false))
	{
		for (USceneComponent* Child : Parent->GetAttachChildren())
		{
			UFXSystemComponent* FX = Cast<UFXSystemComponent>(Child);
			if (FX && FX->ComponentHasTag(Tag))
			{
				return FX;
			}
		}
	}

	// 2) 구간 도중 무기가 바뀌었으면 액터 전체에서
	if (AActor* Owner = MeshComp->GetOwner())
	{
		TArray<UFXSystemComponent*> FXComponents;
		Owner->GetComponents(FXComponents);
		for (UFXSystemComponent* FX : FXComponents)
		{
			if (FX && FX->ComponentHasTag(Tag))
			{
				return FX;
			}
		}
	}
	return nullptr;
}