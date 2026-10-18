#include "DataBridgeTransaction.h"

#include "DataBridgeTypes.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "Misc/ITransaction.h"
#include "ScopedTransaction.h"

bool FDataBridgeTransaction::CanBegin(FDataBridgeContext& Ctx)
{
	if (!GEditor)
	{
		Ctx.AddError(TEXT("Editor is not available; cannot open a transaction"));
		return false;
	}
	if (GEditor->IsTransactionActive())
	{
		Ctx.AddError(TEXT("Another editor transaction is in progress; import commit is not allowed inside it"));
		return false;
	}
	return true;
}

FDataBridgeTransaction::FDataBridgeTransaction(const FText& Description)
{
	ensureMsgf(GEditor && !GEditor->IsTransactionActive(), TEXT("DataBridge: call CanBegin() before opening a transaction"));

	Scoped = MakeUnique<FScopedTransaction>(Description);

	// 열려 있는 동안 GUndo가 이번 트랜잭션. 번호를 기억해 둔다.
	if (GUndo)
	{
		TransactionId = GUndo->GetContext().TransactionId;
	}
}

FDataBridgeTransaction::~FDataBridgeTransaction()
{
	End();
}

void FDataBridgeTransaction::End()
{
	Scoped.Reset();
}

EDataBridgeUndoResult FDataBridgeTransaction::UndoIfOwned()
{
	End();

	if (!GEditor || !GEditor->Trans || !TransactionId.IsValid())
	{
		return EDataBridgeUndoResult::NotOwned;
	}

	// 변경이 없던 트랜잭션은 End 시점에 엔진이 버린다.
	// 그 상태로 Undo하면 사용자의 직전 작업이 되돌려지므로 반드시 번호를 비교한다.
	const FTransactionContext Top = GEditor->Trans->GetUndoContext(true);
	if (Top.TransactionId != TransactionId)
	{
		return EDataBridgeUndoResult::NotOwned;
	}

	return GEditor->UndoTransaction(/*bCanRedo*/ false)
		? EDataBridgeUndoResult::Undone
		: EDataBridgeUndoResult::Failed;
}