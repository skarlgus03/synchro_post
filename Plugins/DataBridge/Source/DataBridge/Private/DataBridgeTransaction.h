#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

class FScopedTransaction;
class FDataBridgeContext;

enum class EDataBridgeUndoResult : uint8
{
	Undone,    // 이번 트랜잭션을 되돌림
	NotOwned,  // 맨 위 기록이 이번 것이 아님 (변경이 없어 엔진이 버렸거나 다른 기록이 위에 있음) → 아무것도 안 함
	Failed,    // 이번 것이 맞지만 엔진이 Undo를 거부함 (패키지 저장 중, GC 중 등)
};

// 에디터 트랜잭션 하나를 소유하고, 실패 시 "자기 기록일 때만" 되돌린다.
// FScopedTransaction::Cancel()은 변경을 되돌리지 않으므로 사용하지 않는다.
class FDataBridgeTransaction : public FNoncopyable
{
public:
	// 다른 트랜잭션이 진행 중이면 false (중첩 금지). 생성 전에 반드시 호출.
	static bool CanBegin(FDataBridgeContext& Ctx);

	explicit FDataBridgeTransaction(const FText& Description);
	~FDataBridgeTransaction();

	// 기록 종료. 여러 번 불러도 안전.
	void End();

	// 기록을 종료한 뒤, Undo 맨 위가 이번 트랜잭션일 때만 Redo 없이 되돌린다.
	EDataBridgeUndoResult UndoIfOwned();

	const FGuid& GetTransactionId() const { return TransactionId; }

private:
	TUniquePtr<FScopedTransaction> Scoped;
	FGuid TransactionId;
};