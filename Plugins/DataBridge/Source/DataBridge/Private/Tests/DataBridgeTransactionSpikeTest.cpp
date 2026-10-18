#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgeTransaction.h"
#include "DataBridgeTypes.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "ScopedTransaction.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgeSpike
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	// /Temp 아래 고유 패키지에 "부모(Value=10) → [자식(Value=1)]"을 만든다.
	// Standalone이 아니므로 참조가 사라지면 GC가 수거한다.
	UDataBridgeTestAsset* MakeTestAsset()
	{
		const FString PackageName = FString::Printf(TEXT("/Temp/DataBridgeSpike/Spike_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits));

		UPackage* Package = CreatePackage(*PackageName);
		UDataBridgeTestAsset* Asset = NewObject<UDataBridgeTestAsset>(Package, TEXT("SpikeAsset"), RF_Transactional);
		UDataBridgeTestChild* Child = NewObject<UDataBridgeTestChild>(Asset, NAME_None, RF_Transactional);

		Asset->Value = 10;
		Child->Value = 1;
		Asset->Children.Add(Child);
		return Asset;
	}
}

// 실험 A: 자식 값 변경 + 자식 교체 후 Undo → "원래 그 자식"이 돌아오는가
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeSpikeUndoInstancedTest,
	"DataBridge.Spike.A_UndoRestoresOriginalInstancedChild", DataBridgeSpike::TestFlags)

	bool FDataBridgeSpikeUndoInstancedTest::RunTest(const FString& Parameters)
{
	FDataBridgeContext Ctx;
	if (!TestTrue(TEXT("Can begin transaction"), FDataBridgeTransaction::CanBegin(Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	TStrongObjectPtr<UDataBridgeTestAsset> Asset(DataBridgeSpike::MakeTestAsset());
	TStrongObjectPtr<UDataBridgeTestChild> OriginalChild(Asset->Children[0].Get());
	const FName OriginalChildName = OriginalChild->GetFName();
	TStrongObjectPtr<UDataBridgeTestChild> NewChild;

	FGuid OurId;
	EDataBridgeUndoResult Result = EDataBridgeUndoResult::Failed;
	{
		FDataBridgeTransaction Transaction(NSLOCTEXT("DataBridgeTests", "SpikeA", "DataBridge Spike A"));
		OurId = Transaction.GetTransactionId();
		TestTrue(TEXT("Transaction id is valid"), OurId.IsValid());

		TestTrue(TEXT("Asset recorded by Modify()"), Asset->Modify());
		TestTrue(TEXT("Original child recorded by Modify()"), OriginalChild->Modify());

		OriginalChild->Value = 2;   // 기존 자식 값 변경
		Asset->Value = 20;

		NewChild.Reset(NewObject<UDataBridgeTestChild>(Asset.Get(), NAME_None, RF_Transactional));
		NewChild->Value = 3;
		Asset->Children.Reset();    // 자식 교체
		Asset->Children.Add(NewChild.Get());

		Result = Transaction.UndoIfOwned();
	}

	TestTrue(TEXT("Undo result is Undone"), Result == EDataBridgeUndoResult::Undone);

	TestEqual(TEXT("Asset value restored"), Asset->Value, 10);
	if (TestEqual(TEXT("Child count restored"), Asset->Children.Num(), 1))
	{
		TestTrue(TEXT("Parent points to the ORIGINAL child instance"), Asset->Children[0].Get() == OriginalChild.Get());
	}
	TestEqual(TEXT("Original child value restored"), OriginalChild->Value, 1);
	TestTrue(TEXT("Original child outer unchanged"), OriginalChild->GetOuter() == Asset.Get());
	TestTrue(TEXT("Original child name unchanged"), OriginalChild->GetFName() == OriginalChildName);
	TestTrue(TEXT("Original child is valid (not garbage)"), IsValid(OriginalChild.Get()));
	TestTrue(TEXT("Our transaction cannot be redone"), GEditor->Trans->GetRedoContext().TransactionId != OurId);

	return true;
}

// 실험 B: 변경 없는 트랜잭션 → 엉뚱한(사용자) 기록을 되돌리지 않는가
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeSpikeEmptyTransactionTest,
	"DataBridge.Spike.B_EmptyTransactionDoesNotUndoOthers", DataBridgeSpike::TestFlags)

	bool FDataBridgeSpikeEmptyTransactionTest::RunTest(const FString& Parameters)
{
	FDataBridgeContext Ctx;
	if (!TestTrue(TEXT("Can begin transaction"), FDataBridgeTransaction::CanBegin(Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	TStrongObjectPtr<UDataBridgeTestAsset> Asset(DataBridgeSpike::MakeTestAsset());

	// 사용자 작업 흉내: 값을 99로
	FDataBridgeTransaction UserEdit(NSLOCTEXT("DataBridgeTests", "SpikeBUser", "DataBridge Spike B (user edit)"));
	Asset->Modify();
	Asset->Value = 99;
	UserEdit.End();

	// 우리 작업: 아무것도 바꾸지 않음
	EDataBridgeUndoResult Result = EDataBridgeUndoResult::Undone;
	{
		FDataBridgeTransaction Ours(NSLOCTEXT("DataBridgeTests", "SpikeBOurs", "DataBridge Spike B (ours, empty)"));
		Result = Ours.UndoIfOwned();
	}

	// 위험이 실제로 존재했는지: 빈 기록이 버려져서 맨 위가 사용자 작업이어야 한다
	TestTrue(TEXT("Empty transaction was discarded; top is the user edit"),
		GEditor->Trans->GetUndoContext(true).TransactionId == UserEdit.GetTransactionId());

	TestTrue(TEXT("Undo result is NotOwned"), Result == EDataBridgeUndoResult::NotOwned);
	TestEqual(TEXT("User edit is kept"), Asset->Value, 99);

	// 뒷정리: 사용자 작업 흉내 기록을 Undo 목록에서 제거
	TestTrue(TEXT("Cleanup: user edit undone"), UserEdit.UndoIfOwned() == EDataBridgeUndoResult::Undone);
	TestEqual(TEXT("Cleanup: value back to 10"), Asset->Value, 10);

	return true;
}

// 실험 C: 다른 트랜잭션이 열려 있으면 시작을 거부하는가
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeSpikeNestedTransactionTest,
	"DataBridge.Spike.C_RejectsWhileAnotherTransactionActive", DataBridgeSpike::TestFlags)

	bool FDataBridgeSpikeNestedTransactionTest::RunTest(const FString& Parameters)
{
	{
		FScopedTransaction Outer(NSLOCTEXT("DataBridgeTests", "SpikeCOuter", "DataBridge Spike C (outer)"));

		FDataBridgeContext Ctx;
		TestFalse(TEXT("CanBegin rejects while another transaction is active"), FDataBridgeTransaction::CanBegin(Ctx));
		TestTrue(TEXT("Rejection is reported as an error"), Ctx.HasErrors());
	}
	// Outer는 변경이 없으므로 엔진이 버린다 (Undo 목록에 남지 않음)

	TestFalse(TEXT("No transaction left active"), GEditor->IsTransactionActive());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS