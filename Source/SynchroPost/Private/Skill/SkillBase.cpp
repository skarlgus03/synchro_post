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
#include "Math/GridMath.h"
#include "Unit/StateComponent.h"
#include "Skill/SkillTargetRules.h"


const FSkillData& USkillBase::GetSkillData() const
{
	if (!SkillDataAsset)
	{
		UE_LOG(LogSP, Error, TEXT("[Skill] %s: SkillDataAsset 없음 (InitializeSkill 전에 호출됨) → 빈 데이터 반환"), *GetName());
		static const FSkillData Empty;
		return Empty;
	}
	return SkillDataAsset->SkillData;
}

bool USkillBase::IsWithinCastRange(const FIntPoint& TargetCoord, const int32& CastRange, const FSkillExecutionContext& Context) const
{
	
	FIntPoint CasterCoord = Context.CasterCoordinate;

	int32 Distance = FMath::Abs(TargetCoord.X - CasterCoord.X) + FMath::Abs(TargetCoord.Y - CasterCoord.Y);
	return Distance <= CastRange;

}

bool USkillBase::IsValidSingleTargetTile(const FIntPoint& Coord, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& TargetingRule = GetTargetingRule();

	return IsWithinCastRange(Coord, TargetingRule.CastRange, Context)
		&& IsTileAllowed(Coord, TargetingRule.SelectionMask, Context);
}


void USkillBase::InitializeSkill(USkillDataAsset* InSkillDataAsset)
{
	if (!InSkillDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillBase::InitializeSkill - InSkillDataAsset is null."));
		return;
	}

	SkillDataAsset = InSkillDataAsset;

	CurrentCooldown = 0;

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
	SkillPayload.Context = Context;

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

	if (AUnit* Caster = Event.Source.Get())
	{
		const FSkillExecutionContext& Ctx = SkillPayload->Context;
		// 자기 칸을 조준한 스킬(자기 중심 광역기)은 돌지 않는다
		if (Ctx.AimCoordinate != Ctx.CasterCoordinate)
		{
			Caster->PresentFace(Ctx.CasterCoordinate, Ctx.AimCoordinate);
		}
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

bool USkillBase::IsTileAllowed(const FIntPoint& Coord, int32 Mask, const FSkillExecutionContext& Context) const
{
	const UGridManager* GridManager = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!GridManager)
	{
		return false;
	}

	// GetUnitAt이 아니라 GetOccupantAt — 장애물도 판정 대상
	const ETargetRelation Relation = SkillTargetRules::GetRelation(
		GridManager->GetOccupantAt(Coord), GetOwnerUnit(), Context.CasterFaction);
	return SkillTargetRules::Allows(Mask, Relation);
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
	CurrentCooldown = GetSkillData().BaseCooldown;

	ApplyEffectToTargets(TargetData, Context);
}

bool USkillBase::CanExecuteOnTarget_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule();

	if (TargetData.SelectedTiles.Num() != Rule.RequiredTileSelectionCount)
	{
		return false;
	}
	

	for (const FIntPoint& Coord : TargetData.SelectedTiles)
	{
		if (!IsWithinCastRange(Coord, Rule.CastRange, Context))
		{
			return false;
		}

		if (!IsTileAllowed(Coord, Rule.SelectionMask, Context))
		{
			return false;
		}
	}

	return true;
}


TArray<FIntPoint> USkillBase::GetAffectedTiles_Implementation(const FIntPoint& TargetCoord, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule();

	if (Rule.RangePatternOffsets.Num() == 0)
	{
		return { TargetCoord };
	}

	FIntPoint Direction = FIntPoint::ZeroValue;
	if (Rule.bRotatePatternToCasterDirection)
	{
		Direction = GridMath::ToCardinalDirection(TargetCoord - Context.CasterCoordinate);
	}

	TArray<FIntPoint> AffectedTiles;
	for (const FIntPoint& Offset : Rule.RangePatternOffsets)
	{
		FIntPoint AffectedCoord = TargetCoord + GridMath::RotateOffset(Offset, Direction);
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

	const FSkillTargetingRule& Rule = GetTargetingRule();
		
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

void USkillBase::DecreaseCooldown()
{
	if (CurrentCooldown > 0)
	{
		--CurrentCooldown;
	}
}

TArray<FAffectedTarget> USkillBase::GatherAffectedTargets(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context)
{
	TArray<FAffectedTarget> Result;

	AUnit* Caster = GetOwnerUnit();
	UGridManager* GridManager = Caster ? Caster->GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!GridManager)
	{
		return Result;
	}

	const FSkillTargetingRule& Rule = GetTargetingRule();

	TSet<FIntPoint> Visited;

	for (const FIntPoint& Selected : TargetData.SelectedTiles)
	{
		for (const FIntPoint& Coord : GetAffectedTiles(Selected, Context))
		{
			if (Visited.Contains(Coord)) { continue; }
			Visited.Add(Coord);

			AActor* Occupant = GridManager->GetOccupantAt(Coord);
			const ETargetRelation Relation = SkillTargetRules::GetRelation(Occupant, Caster, Context.CasterFaction);

			// 빈 칸은 효과를 받을 대상이 없다 (TargetMask의 EmptyTile은 의미 없음)
			if (Relation == ETargetRelation::None) { continue; }
			if (!SkillTargetRules::Allows(Rule.TargetMask, Relation)) { continue; }

			FAffectedTarget& Target = Result.AddDefaulted_GetRef();
			Target.Actor = Occupant;
			Target.Coordinate = Coord;
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
	const FSkillData& Data = GetSkillData();

	float Total = 0.f;
	for (const TPair<FGameplayTag, int32>& Pair : Data.DamageCoefficients)
	{
		Total += CasterStat->GetStat(Pair.Key) * StatMath::PercentToFloat(Pair.Value);
	}

	return FMath::RoundToInt(Total);
}

FCombatEventTarget USkillBase::ApplyToTarget(const FAffectedTarget& Target, const FSPHealthActionData& ActionData)
{
	FCombatEventTarget Result;
	Result.Coordinate = Target.Coordinate;
	Result.ActionData = ActionData;

	AActor* Actor = Target.Actor.Get();
	if (!Actor || !Actor->Implements<UDamageable>())
	{
		return Result;   // 피해를 받을 수 없는 대상 — 칸만 채운 결과
	}

	Result.Target = Actor;
	Result.HealthBeforeChange = IDamageable::Execute_GetCurrentHealth(Actor);
	if (Result.HealthBeforeChange != 0)
	{
		IDamageable::Execute_ApplyHealthChange(Actor, ActionData);
	}
	Result.HealthAfterChange = IDamageable::Execute_GetCurrentHealth(Actor);

	return Result;
}

TArray<FCombatEventTarget> USkillBase::ApplyStandardEffect(const FSkillTargetData& TargetData,
	const FSkillExecutionContext& Context)
{
	TArray<FCombatEventTarget> Results;

	const FSkillData& Data = GetSkillData();

	FSPHealthActionData ActionData;
	ActionData.Amount = CalculateSkillAmount(Context);
	ActionData.ActionTypeTags = Data.GetActionTypeTags();  
	ActionData.Causer = GetOwnerUnit();

	for (const FAffectedTarget& Target : GatherAffectedTargets(TargetData, Context))
	{
		Results.Add(ApplyToTarget(Target, ActionData));

		// 상태이상은 유닛만 적용함 (유닛만 상태이상 컴포넌트를 가지고 있음)
		if (AUnit* Unit = Cast<AUnit>(Target.Actor.Get()))
		{
			ApplyStatusEffectsToTarget(Unit, Context);
		}
	}

	return Results;
}

void USkillBase::ApplyStatusEffectsToTarget(AUnit* TargetUnit, const FSkillExecutionContext& Context)
{
	if (!TargetUnit || TargetUnit->IsDead())
	{
		return;
	}

	UStateComponent* StateComp = TargetUnit->GetStateComponent();
	if (!StateComp)
	{
		return;
	}
	for (const FSkillStatusEffectSpec& Spec : GetSkillData().StatusEffects)
	{
		const bool bRollSuccess = FMath::RandHelper(100) < Spec.ChancePercent;

		if (bRollSuccess)
		{
			StateComp->ApplyStatusEffect(Spec.EffectData, Spec.Duration, GetOwnerUnit());
		}
	}
}


AUnit* USkillBase::GetOwnerUnit() const
{
	return OwnerComp ? Cast<AUnit>(OwnerComp->GetOwner()) : nullptr;
}
