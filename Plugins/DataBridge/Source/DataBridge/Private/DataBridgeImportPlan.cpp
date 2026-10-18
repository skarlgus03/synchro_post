#include "DataBridgeImportPlan.h"

// ---------------------------------------------------------------------------
// FDataBridgeImportPlan
// ---------------------------------------------------------------------------

int32 FDataBridgeImportPlan::Num(EDataBridgeImportAction Action) const
{
	int32 Count = 0;
	for (const FDataBridgeImportEntry& Entry : Entries)
	{
		if (Entry.Action == Action)
		{
			++Count;
		}
	}
	return Count;
}

const FDataBridgeImportEntry* FDataBridgeImportPlan::FindByPackageName(const FString& PackageName) const
{
	return Entries.FindByPredicate([&PackageName](const FDataBridgeImportEntry& Entry)
		{
			return Entry.PackageName.Equals(PackageName, ESearchCase::IgnoreCase);
		});
}

FString FDataBridgeImportPlan::Describe() const
{
	FString Out;
	for (const FDataBridgeImportEntry& Entry : Entries)
	{
		Out += FString::Printf(TEXT("assets[%d] %s %s (%s)"),
			Entry.SourceIndex,
			Entry.Action == EDataBridgeImportAction::Create ? TEXT("Create") : TEXT("Update"),
			*Entry.PackageName,
			*GetNameSafe(Entry.Class.Get()));
		Out += LINE_TERMINATOR;
	}
	return Out;
}
