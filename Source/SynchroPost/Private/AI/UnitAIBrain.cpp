#include "AI/UnitAIBrain.h"
#include "Unit/Unit.h"
#include "Framework/TurnManager.h"
#include "Framework/SPGameState.h"
#include "Framework/TurnStateComponent.h"
#include "TimerManager.h"
#include "SynchroPost.h"

UWorld* UUnitAIBrain::GetWorld() const
{
	// CDO와 DA 안의 원본은 월드가 없다
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	const UObject* Outer = GetOuter();
	return Outer ? Outer->GetWorld() : nullptr;
}

AUnit* UUnitAIBrain::GetOwnerUnit() const
{
	return Cast<AUnit>(GetOuter());
}

void UUnitAIBrain::BeginTurn()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSP, Error, TEXT("[AI] %s: 월드 없음 — 턴 진행 불가"), *GetNameSafe(GetOwnerUnit()));
		return;
	}

	bTurnActive = true;

	// 턴 시작 브로드캐스트 도중에 다음 턴이 시작되는 재진입을 막기 위해 한 틱 미룬다
	World->GetTimerManager().SetTimerForNextTick(this, &UUnitAIBrain::HandleDeferredExecute);
}

void UUnitAIBrain::HandleDeferredExecute()
{
	if (!bTurnActive)
	{
		return;
	}

	const AUnit* Unit = GetOwnerUnit();
	if (!Unit || Unit->IsDead())
	{
		FinishTurn();
		return;
	}

	ExecuteTurn();
}

void UUnitAIBrain::ExecuteTurn_Implementation()
{
	UE_LOG(LogSP, Log, TEXT("[AI] %s: 행동 없음, 턴 넘김"), *GetNameSafe(GetOwnerUnit()));
	FinishTurn();
}

void UUnitAIBrain::FinishTurn()
{
	if (!bTurnActive)
	{
		return; // 중복 호출
	}
	// EndCurrentUnitTurn 안에서 같은 유닛의 턴이 바로 다시 시작될 수 있으므로 먼저 내린다
	bTurnActive = false;

	AUnit* Unit = GetOwnerUnit();
	UWorld* World = GetWorld();
	if (!Unit || !World)
	{
		return;
	}

	const ASPGameState* GS = World->GetGameState<ASPGameState>();
	const UTurnStateComponent* TurnState = GS ? GS->GetTurnStateComponent() : nullptr;
	if (!TurnState || TurnState->GetCurrentUnit() != Unit)
	{
		UE_LOG(LogSP, Warning, TEXT("[AI] %s: 이미 내 턴이 아님 — 종료 요청 무시"), *GetNameSafe(Unit));
		return;
	}

	if (UTurnManager* TurnManager = World->GetSubsystem<UTurnManager>())
	{
		TurnManager->EndCurrentUnitTurn();
	}
}