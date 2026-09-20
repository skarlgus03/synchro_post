#include "Skill/Step_PlayMontage.h"
#include "Unit/Unit.h"
#include "Unit/SkillComponent.h"
#include "SynchroPost.h"

void UStep_PlayMontage::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;

	AUnit* Caster = Ctx.Caster.Get();
	if (!Caster || !Montage)
	{
		Finish();
		return;
	}

	USkeletalMeshComponent* MeshComp = Caster->FindComponentByClass<USkeletalMeshComponent>();
	UAnimInstance* AnimInst = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		Finish();
		return;
	}
	const float Duration = AnimInst->Montage_Play(Montage, PlayRate);
	if (Duration <= 0.f)
	{
		// 재생 실패. 연출을 멈추는 대신 그냥 다음 줄로 넘긴다.
		UE_LOG(LogTemp, Warning, TEXT("[SkillStep] 몽타주 재생 실패: %s"), *GetNameSafe(Montage));
		Finish();
		return;
	}

	if (!StartSection.IsNone())
	{
		AnimInst->Montage_JumpToSection(StartSection, Montage);
	}

	if (EndCondition == ESkillStepMontageEnd::Immediate)
	{
		Finish();
		return;
	}

	BoundAnimInstance = AnimInst;


	// 몽타주 종료 델리게이트는 반드시 바인딩해야 한다. (중단이어도 다음 줄로 넘어가도록)
	// CueReceived일 때도 몽타주 종료를 백스톱으로 삼는다. (큐가 안 날아오면 턴제 게임이 정지한다.)
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UStep_PlayMontage::HandleMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	if (EndCondition == ESkillStepMontageEnd::CueReceived)
	{
		USkillComponent* SkillComp = Ctx.OwnerComp.Get();
		if (!SkillComp || !WaitCueTag.IsValid())
		{
			UE_LOG(LogSP, Warning, TEXT("[SP] PlayMontage: 큐 대기 불가 - 몽타주 종료로 대체"));
			return;   // 백스톱(몽타주 종료)에 맡긴다
		}
		BoundSkillComp = SkillComp;
		CueHandle = SkillComp->OnSkillCue.AddUObject(this, &UStep_PlayMontage::HandleSkillCue);
	}
}

void UStep_PlayMontage::HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted)
{
	// 큐로 이미 끝난 뒤에 도착한 몽타주 종료. 정상 흐름이므로 조용히 무시.
	if (IsFinished())
	{
		UnbindAll();
		return;
	}

	if (EndCondition == ESkillStepMontageEnd::CueReceived)
	{
		UE_LOG(LogSP, Warning,
			TEXT("[SP] 큐(%s)가 오지 않은 채 몽타주가 끝남 - 노티파이 배치 확인 필요"),
			*WaitCueTag.ToString());
	}

	UnbindAll();
	Finish();
}

void UStep_PlayMontage::HandleSkillCue(FGameplayTag CueTag)
{
	if (!CueTag.MatchesTag(WaitCueTag)) { return; }

	UE_LOG(LogSP, Verbose, TEXT("[SP] 큐 수신: %s"), *CueTag.ToString());
	UnbindAll();
	Finish();
}

void UStep_PlayMontage::Abort()
{
	// 재생 중인 몽타주가 있으면 중단한다.
	if (UAnimInstance* AnimInst = BoundAnimInstance.Get())
	{
		if (Montage)
		{
			AnimInst->Montage_Stop(0.15f, Montage);
		}
	}
	UnbindAll();
}

void UStep_PlayMontage::UnbindAll()
{
	if (USkillComponent* SkillComp = BoundSkillComp.Get())
	{
		SkillComp->OnSkillCue.Remove(CueHandle);
	}
	CueHandle.Reset();
	BoundSkillComp.Reset();

	// 몽타주 종료 델리게이트도 같이 해제. 빈 델리게이트를 덮어씌우면 된다.
	if (UAnimInstance* AnimInst = BoundAnimInstance.Get())
	{
		if (Montage)
		{
			FOnMontageEnded EmptyDelegate;
			AnimInst->Montage_SetEndDelegate(EmptyDelegate, Montage);
		}
	}
	BoundAnimInstance.Reset();
}