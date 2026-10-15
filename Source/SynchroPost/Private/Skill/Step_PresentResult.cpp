#include "Skill/Step_PresentResult.h"
#include "Interface/Damageable.h"
#include "Unit/SkillComponent.h"
#include "Types/SPGameplayTags.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "SynchroPost.h"

namespace
{
	// Total을 Count 토막으로 나눈 Index번째. 나머지는 마지막 토막에 → 합이 정확히 Total
	int32 SplitPiece(int32 Total, int32 Count, int32 Index)
	{
		const int32 Base = Total / Count;   // 0 쪽으로 버림 → 음수(피해)도 대칭
		return (Index == Count - 1) ? Total - Base * (Count - 1) : Base;
	}
}

void UStep_PresentResult::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;
	BuildPlan();

	// 트랙 없음, 또는 트랙이 맡은 조각이 없음(빗나감·전부 면역) → 기다릴 이유 없음
	if (AllTracksDone())
	{
		FlushRemaining(/*bWarn*/ Tracks.Num() > 0);
		Complete();
		return;
	}

	USkillComponent* SkillComp = Ctx.OwnerComp.Get();
	if (!SkillComp)
	{
		UE_LOG(LogSP, Warning, TEXT("[SP] 결과 표시: 큐를 받을 SkillComponent 없음 → 한 번에 표시"));
		FlushRemaining(false);
		Complete();
		return;
	}

	BoundSkillComp = SkillComp;
	CueHandle = SkillComp->OnSkillCue.AddUObject(this, &UStep_PresentResult::HandleSkillCue);

	if (UWorld* World = GetContextWorld())
	{
		World->GetTimerManager().SetTimer(TimeoutHandle, this, &UStep_PresentResult::HandleTimeout, MaxWaitSeconds, false);
	}
}

void UStep_PresentResult::BuildPlan()
{
	Pieces.Reset();
	TrackPlays.Reset();
	TrackPlays.SetNum(Tracks.Num());
	RawHealth.Reset();

	const TArray<FCombatEventTarget>& Targets = Ctx.Payload.Targets;
	for (int32 TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
	{
		const FCombatEventTarget& Target = Targets[TargetIndex];
		RawHealth.Add(Target.HealthBeforeChange);

		TArray<FSPHealthActionData> Hits = Target.Hits;
		// Hits 없이 체력만 바뀐 결과(다른 경로)는 변화량을 타격 하나로 취급
		if (Hits.Num() == 0 && Target.HealthAfterChange != Target.HealthBeforeChange)
		{
			FSPHealthActionData& Fallback = Hits.AddDefaulted_GetRef();
			Fallback = Target.ActionData;
			Fallback.Parts.Reset();
			Fallback.Amount = FMath::Abs(Target.HealthAfterChange - Target.HealthBeforeChange);
		}

		for (FSPHealthActionData& Hit : Hits)
		{
			Hit.EnsureParts();
			const bool bHeal = Hit.ActionTypeTags.HasTag(SPTags::Action::Heal);

			for (const FDamagePart& Part : Hit.Parts)
			{
				if (Part.Amount == 0)
				{
					continue;   // 면역 등 — 지금은 표시하지 않음
				}

				FPiecePlay Piece;
				Piece.TargetIndex = TargetIndex;
				Piece.Signed = bHeal ? Part.Amount : -Part.Amount;
				Piece.Tags = Part.Tags;
				Piece.bCritical = Hit.bIsCriticalHit;   // 치명타는 타격 단위
				const int32 PieceIndex = Pieces.Add(Piece);

				// 위 트랙부터 먼저 가져간다. 아무도 안 가져가면 끝날 때 몰아서 표시
				const int32 TrackIndex = Tracks.IndexOfByPredicate([&Part](const FResultTrack& Track)
					{
						return Track.HitFilter.IsEmpty() || Part.Tags.HasAny(Track.HitFilter);
					});
				if (TrackIndex != INDEX_NONE)
				{
					TrackPlays[TrackIndex].PieceIndices.Add(PieceIndex);
				}
			}
		}
	}
}

bool UStep_PresentResult::AllTracksDone() const
{
	for (int32 i = 0; i < Tracks.Num(); ++i)
	{
		if (TrackPlays[i].PieceIndices.Num() > 0 && TrackPlays[i].Received < Tracks[i].CueCount)
		{
			return false;
		}
	}
	return true;
}

void UStep_PresentResult::HandleSkillCue(FGameplayTag CueTag)
{
	if (IsFinished())
	{
		return;
	}

	for (int32 i = 0; i < Tracks.Num(); ++i)
	{
		const FResultTrack& Track = Tracks[i];
		FTrackPlay& TrackPlay = TrackPlays[i];
		if (!CueTag.MatchesTagExact(Track.CueTag) || TrackPlay.Received >= Track.CueCount)
		{
			continue;   // 다른 트랙의 큐, 또는 이미 다 받은 트랙 (남는 큐는 무시)
		}

		for (const int32 PieceIndex : TrackPlay.PieceIndices)
		{
			FPiecePlay& Piece = Pieces[PieceIndex];
			ShowAmount(Piece, SplitPiece(Piece.Signed, Track.CueCount, TrackPlay.Received));
		}
		++TrackPlay.Received;
	}

	if (AllTracksDone())
	{
		FlushRemaining(/*bWarn*/ true);   // 트랙이 안 맡은 조각이 남았으면 경고와 함께 표시
		Complete();
	}
}

void UStep_PresentResult::HandleTimeout()
{
	if (IsFinished())
	{
		return;
	}
	UE_LOG(LogSP, Warning, TEXT("[SP] 결과 표시: %.1f초 안에 큐가 다 오지 않음 → 남은 토막 몰아서 표시"), MaxWaitSeconds);
	FlushRemaining(true);
	Complete();
}

void UStep_PresentResult::ShowAmount(FPiecePlay& Piece, int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}
	Piece.Shown += Amount;

	const FCombatEventTarget& Target = Ctx.Payload.Targets[Piece.TargetIndex];
	int32& Raw = RawHealth[Piece.TargetIndex];
	Raw += Amount;

	// 숫자는 계산값 그대로, 체력바는 서버의 전~후 범위 안으로 (오버킬·과치유여도 끝은 서버 값)
	const int32 Lo = FMath::Min(Target.HealthBeforeChange, Target.HealthAfterChange);
	const int32 Hi = FMath::Max(Target.HealthBeforeChange, Target.HealthAfterChange);
	const int32 BarHealth = FMath::Clamp(Raw, Lo, Hi);

	AActor* Actor = Target.Target.Get();
	if (Actor && Actor->Implements<UDamageable>())
	{
		IDamageable::Execute_ApplyVisualDamage(Actor, Amount, BarHealth, Piece.bCritical, Piece.Tags);
	}
}

void UStep_PresentResult::FlushRemaining(bool bWarn)
{
	for (FPiecePlay& Piece : Pieces)
	{
		const int32 Remaining = Piece.Signed - Piece.Shown;
		if (Remaining == 0)
		{
			continue;
		}
		if (bWarn)
		{
			UE_LOG(LogSP, Warning, TEXT("[SP] 결과 표시: 표시 안 된 조각 %d (%s) → 몰아서 표시. 트랙 필터·큐 확인"),
				Remaining, *Piece.Tags.ToStringSimple());
		}
		ShowAmount(Piece, Remaining);
	}
}

void UStep_PresentResult::Abort()
{
	if (!IsFinished())
	{
		FlushRemaining(false);   // 스킵·타임아웃이어도 체력바는 서버 값으로 끝나게
	}
	Unbind();
}

void UStep_PresentResult::Complete()
{
	Unbind();
	Finish();
}

void UStep_PresentResult::Unbind()
{
	if (USkillComponent* SkillComp = BoundSkillComp.Get())
	{
		SkillComp->OnSkillCue.Remove(CueHandle);
	}
	CueHandle.Reset();
	BoundSkillComp.Reset();

	if (UWorld* World = GetContextWorld())
	{
		World->GetTimerManager().ClearTimer(TimeoutHandle);
	}
}