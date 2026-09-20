#include "Skill/SkillBase.h"
#include "Unit/Unit.h"
#include "Skill/SkillDataAsset.h"
#include "Unit/SkillComponent.h"
#include "Framework/CombatEventComponent.h"
#include "LevelSequence.h"
#include "Framework/GridManager.h"
#include "Skill/SkillPresentation.h"
#include "Unit/StatComponent.h"
#include "SynchroPost.h"


int32 USkillBase::GetCurrentCooldown(const FGameplayTagContainer& StatusTags) const
{
	const int32 CurrentStateIndex = DetermineCurrentIndex(StatusTags);

	if (!CurrentCooldown.IsValidIndex(CurrentStateIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillBase::GetCurrentCooldown - Invalid CurrentStateIndex: %d"), CurrentStateIndex);
		return 0; // 또는 적절한 기본값 반환
	}

	return CurrentCooldown[CurrentStateIndex];
}

const FSkillData& USkillBase::GetSkillDataByIndex(int32 Index) const
{
	if (!SkillDataAsset || !SkillDataAsset->SkillDataArray.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillBase::GetSkillDataByIndex - Invalid Index: %d"), Index);
		static FSkillData DefaultSkillData; // 기본값 반환
		return DefaultSkillData;
	}

	return SkillDataAsset->SkillDataArray[Index];
}

bool USkillBase::IsWithinCastRange(const FIntPoint& TargetCoord, const int32& CastRange, const FSkillExecutionContext& Context) const
{
	
	FIntPoint CasterCoord = Context.CasterCoordinate;

	int32 Distance = FMath::Abs(TargetCoord.X - CasterCoord.X) + FMath::Abs(TargetCoord.Y - CasterCoord.Y);
	return Distance <= CastRange;

}

bool USkillBase::MatchesFaction(ESkillTargetFaction SkillTargetFaction, const FSkillExecutionContext& Context, EFaction TargetUnitFaction) const
{
	
	if (SkillTargetFaction == ESkillTargetFaction::Any)
	{
		return true;
	}

	bool bIsSameFaction = (TargetUnitFaction == Context.CasterFaction);

	if (SkillTargetFaction == ESkillTargetFaction::Ally)
	{
		return bIsSameFaction;
	}
	if (SkillTargetFaction == ESkillTargetFaction::Enemy)
	{
		return !bIsSameFaction;
	}

	return false;
}

bool USkillBase::IsValidSingleTargetTile(const FIntPoint& Coord, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);

	if (!IsWithinCastRange(Coord, Rule.CastRange, Context))
	{
		return false;
	}

	if (Rule.TargetFaction != ESkillTargetFaction::None)
	{
		UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();
		if (!GridManager)
		{
			return false;
		}

		AUnit* TargetUnit = GridManager->GetUnitAt(Coord);
		if (!TargetUnit || !MatchesFaction(Rule.TargetFaction, Context, TargetUnit->GetFaction()))
		{
			return false;
		}
	}

	return true;
}


void USkillBase::InitializeSkill(USkillDataAsset* InSkillDataAsset)
{
	if (!InSkillDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillBase::InitializeSkill - InSkillDataAsset is null."));
		return;
	}

	SkillDataAsset = InSkillDataAsset;

	CurrentCooldown.SetNum(SkillDataAsset->SkillDataArray.Num());
	for (int32& Cooldown : CurrentCooldown)
	{
		Cooldown = 0;
	}

}

void USkillBase::PushSkillCombatEvent(const FSkillExecutionContext& Context, const TArray<FCombatEventTarget>& Targets) 
{
	AUnit* Caster = GetOwnerUnit();

	if (!Caster)
	{
		return;
	}

	UCombatEventComponent* EventComp = Caster->GetCombatEventComponent();
	if (!EventComp)
	{
		return;
	}

	FSkillEventPayload SkillPayload;
	SkillPayload.SkillTag = Context.SkillSlotTag;
	SkillPayload.Targets = Targets;

	FCombatEvent Event;
	Event.Source = Caster;
	Event.Payload = FInstancedStruct::Make(SkillPayload);

	EventComp->PushEvent(Event);
}

void USkillBase::PresentSkillEffect_Implementation(const FCombatEvent& Event)
{
	UE_LOG(LogSP, Verbose, TEXT("[SP] 1. PresentSkillEffect 진입"));


	// 이전 연출이 남아있으면 정리
	if (ActivePresentation)
	{
		ActivePresentation->OnFinished.Unbind();
		ActivePresentation->Abort();
		ActivePresentation = nullptr;
	}
	const FSkillEventPayload* SkillPayload = Event.Payload.GetPtr<FSkillEventPayload>();
	USkillPresentation* Template = SkillDataAsset ? SkillDataAsset->Presentation.Get() : nullptr;

	if (!SkillPayload || !Template)
	{
		// 대본이 없어도 큐는 계속 흘러가야 한다. 여기서 안 알리면 게임이 멈춘다.
		UE_LOG(LogTemp, Warning, TEXT("[Skill] 연출 대본 없음: %s"), *GetNameSafe(SkillDataAsset));
		NotifySkillEffectPresentationFinished();
		return;
	}

	// ── 대본 재생 ────────────────────────────────────────────────
	// DA에 붙어있는 건 모든 유닛이 공유하는 '템플릿'이다.
	// 복제하지 않으면 두 유닛이 같은 스킬을 쓸 때 서로의 진행 상태를 짓밟는다.
	ActivePresentation = DuplicateObject<USkillPresentation>(Template, this);
	if (!ActivePresentation)
	{
		NotifySkillEffectPresentationFinished();
		return;
	}

	FSkillPresentationContext PresentCtx;
	PresentCtx.Caster = Event.Source;
	PresentCtx.OwnerComp = OwnerComp;
	PresentCtx.Payload = *SkillPayload;

	ActivePresentation->OnFinished.BindUObject(this, &USkillBase::HandlePresentationFinished);
	ActivePresentation->Play(PresentCtx);
}

void USkillBase::HandlePresentationFinished()
{
	ActivePresentation = nullptr;

	UE_LOG(LogSP, Verbose, TEXT("[SP] 5. 큐에 완료 통보"));
	
	NotifySkillEffectPresentationFinished();
}

void USkillBase::NotifySkillEffectPresentationFinished() const
{
	
	AUnit* Caster = GetOwnerUnit();
	if (!Caster)
	{
		UE_LOG(LogSP, Error,
			TEXT("[SP] 완료 통보 실패: 시전자 없음 | OwnerComp=%s Skill=%s → 큐가 타임아웃까지 멈춘다"),
			*GetNameSafe(OwnerComp), *GetNameSafe(SkillDataAsset));
		return;
	}

	UCombatEventComponent* EventComp = Caster->GetCombatEventComponent();
	if (!EventComp)
	{
		UE_LOG(LogSP, Error,
			TEXT("[SP] 완료 통보 실패: CombatEventComponent 없음 | Caster=%s Skill=%s → 큐가 타임아웃까지 멈춘다"),
			*GetNameSafe(Caster), *GetNameSafe(SkillDataAsset));
		return;
	}

	EventComp->NotifyPresentationFinished();
}

void USkillBase::ExecuteSkill_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context)
{
	const int32 Index = DetermineCurrentIndex(Context.StateTags);
	const FSkillData& Data = GetCurrentSkillData(Context.StateTags);

	if (CurrentCooldown.IsValidIndex(Index))
	{
		CurrentCooldown[Index] = Data.BaseCooldown;
	}

	ApplyEffectToTargets(TargetData, Context);
}

bool USkillBase::CanExecuteOnTarget_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);

	if (TargetData.SelectedTiles.Num() != Rule.RequiredTileSelectionCount)
	{
		return false;
	}

	// 월드 서브시스템 그리드매니저를 가져온다.
	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager)
	{
		return false;
	}

	for (const FIntPoint& Coord : TargetData.SelectedTiles)
	{

		// 사거리 검증
		if (!IsWithinCastRange(Coord, Rule.CastRange, Context))
		{
			return false;
		}

		// 진영 검증
		if (Rule.TargetFaction != ESkillTargetFaction::None)
		{
			AUnit* TargetUnit = GridManager->GetUnitAt(Coord);
			if (!TargetUnit || !MatchesFaction(Rule.TargetFaction, Context, TargetUnit->GetFaction()))
			{
				return false;
			}
		}
	}

	return true;
}


const FSkillData& USkillBase::GetCurrentSkillData(const FGameplayTagContainer& StateTags) const
{
	
	check(SkillDataAsset);

	const int32 Index = DetermineCurrentIndex(StateTags);

	check(SkillDataAsset->SkillDataArray.IsValidIndex(Index));

	return SkillDataAsset->SkillDataArray[Index];
}

const FSkillTargetingRule& USkillBase::GetTargetingRule(const FGameplayTagContainer& StateTags) const
{
	return GetCurrentSkillData(StateTags).TargetingRule;
}


TArray<FIntPoint> USkillBase::GetAffectedTiles_Implementation(const FIntPoint& TargetCoord, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);

	if (Rule.RangePatternOffsets.Num() == 0)
	{
		return { TargetCoord };
	}

	TArray<FIntPoint> AffectedTiles;
	for (const FIntPoint& Offset : Rule.RangePatternOffsets)
	{
		FIntPoint AffectedCoord = TargetCoord + Offset;
		AffectedTiles.Add(AffectedCoord);
	}
	return AffectedTiles;
}

TArray<FIntPoint> USkillBase::GetRangeTiles(const FSkillExecutionContext& Context) const
{
	TArray<FIntPoint> RangeTiles;

	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();

	if (!GridManager)
	{
		return RangeTiles;
	}

	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);
		
	for (const FTile& Tile : GridManager->GetAllTiles())
	{
		if (IsWithinCastRange(Tile.Coordinate, Rule.CastRange, Context))
		{
			RangeTiles.Add(Tile.Coordinate);
		}
	}

	return RangeTiles;
}

TArray<FIntPoint> USkillBase::GetValidTargetTiles(const FSkillExecutionContext& Context) const
{
	TArray<FIntPoint> ValidTiles;

	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();

	if (!GridManager)
	{
		return ValidTiles;
	}
	for (const FTile& Tile : GridManager->GetAllTiles())
	{
		if (IsValidSingleTargetTile(Tile.Coordinate, Context))
		{
			ValidTiles.Add(Tile.Coordinate);
		}
	}

	return ValidTiles;
}

void USkillBase::DecreaseCooldowns()
{
	for (int32& Cooldown : CurrentCooldown)
	{
		if (Cooldown > 0)
		{
			Cooldown--;
		}
	}
}

TArray<AUnit*> USkillBase::GatherAffectedUnits(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context)
{
	TArray<AUnit*> Result;

	AUnit* Caster = GetOwnerUnit();
	UGridManager* GridManager = Caster ? Caster->GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!GridManager)
	{
		return Result;
	}

	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);
		
	TSet<FIntPoint> Visited;

	for (const FIntPoint& Selected : TargetData.SelectedTiles)
	{
		for (const FIntPoint& Coord : GetAffectedTiles(Selected, Context))
		{
			if (Visited.Contains(Coord)) { continue; }
			Visited.Add(Coord);

			AUnit* Unit = GridManager->GetUnitAt(Coord);
			if (!Unit || Unit->IsDead()) { continue; }

			// 진영 필터 (적 대상 스킬이 아군을 때리지 않게)
			if (!MatchesFaction(Rule.TargetFaction, Context, Unit->GetFaction())) { continue; }

			Result.Add(Unit);
		}
	}

	return Result;
}

int32 USkillBase::CalculateSkillAmount(const FSkillExecutionContext& Context)
{
	AUnit* Caster = GetOwnerUnit();
	UStatComponent* CasterStat = Caster ? Caster->GetStatComponent() : nullptr;
	if (!CasterStat)
	{
		return 0;
	}
	const FSkillData& Data = GetCurrentSkillData(Context.StateTags);

	float Total = 0.f;
	for (const TPair<FGameplayTag, int32>& Pair : Data.DamageCoefficients)
	{
		Total += CasterStat->GetStat(Pair.Key) * StatMath::PercentToFloat(Pair.Value);
	}

	return FMath::RoundToInt(Total);
}

FCombatEventTarget USkillBase::ApplyToTarget(AUnit* TargetUnit, const FSPHealthActionData& ActionData)
{
	FCombatEventTarget Result;
	if (!TargetUnit)
	{
		return Result;
	}
	Result.Coordinate = TargetUnit->GetGridPosition();
	Result.Target = TargetUnit;
	Result.ActionData = ActionData;

	Result.HealthBeforeChange = IDamageable::Execute_GetCurrentHealth(TargetUnit);
	IDamageable::Execute_ApplyHealthChange(TargetUnit, ActionData);
	Result.HealthAfterChange = IDamageable::Execute_GetCurrentHealth(TargetUnit);
	
	return Result;
}

TArray<FCombatEventTarget> USkillBase::ApplyStandardEffect(const FSkillTargetData& TargetData,
	const FSkillExecutionContext& Context)
{
	TArray<FCombatEventTarget> Results;

	const FSkillData& Data = GetCurrentSkillData(Context.StateTags);

	FSPHealthActionData ActionData;
	ActionData.Amount = CalculateSkillAmount(Context);
	ActionData.ActionTypeTags = Data.GetActionTypeTags();  
	ActionData.Causer = GetOwnerUnit();

	for (AUnit* TargetUnit : GatherAffectedUnits(TargetData, Context))
	{
		Results.Add(ApplyToTarget(TargetUnit, ActionData));
	}

	return Results;
}


AUnit* USkillBase::GetOwnerUnit() const
{
	return OwnerComp ? Cast<AUnit>(OwnerComp->GetOwner()) : nullptr;
}
