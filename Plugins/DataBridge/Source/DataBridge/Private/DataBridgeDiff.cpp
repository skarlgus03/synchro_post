#include "DataBridgeDiff.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace DataBridgeDiff
{

	FString Describe(const TSharedPtr<FDataBridgeValue>& Value)
	{
		if (!Value.IsValid())
		{
			return TEXT("(none)");
		}
		switch (Value->Type)
		{
		case EJson::Null:
			return TEXT("null");
		case EJson::Boolean:
			return Value->AsBool() ? TEXT("true") : TEXT("false");
		case EJson::Number:
		{
			const double Number = Value->AsNumber();
			return Number == FMath::RoundToDouble(Number)
				? FString::Printf(TEXT("%.0f"), Number)
				: FString::Printf(TEXT("%g"), Number);
		}
		case EJson::String:
			return FString::Printf(TEXT("\"%s\""), *Value->AsString());
		case EJson::Array:
			return FString::Printf(TEXT("[%d items]"), Value->AsArray().Num());
		case EJson::Object:
			return TEXT("{...}");
		default:
			return TEXT("?");
		}
	}

	void Collect(const TSharedPtr<FDataBridgeValue>& Before, const TSharedPtr<FDataBridgeValue>& After,
		const FString& Path, TArray<FString>& OutChanges, double Tolerance)
	{
		if (!Before.IsValid() && !After.IsValid())
		{
			return;
		}
		if (!Before.IsValid())
		{
			OutChanges.Add(FString::Printf(TEXT("%s: added = %s"), *Path, *Describe(After)));
			return;
		}
		if (!After.IsValid())
		{
			OutChanges.Add(FString::Printf(TEXT("%s: removed (was %s)"), *Path, *Describe(Before)));
			return;
		}
		if (Before->Type != After->Type)
		{
			OutChanges.Add(FString::Printf(TEXT("%s: %s -> %s"), *Path, *Describe(Before), *Describe(After)));
			return;
		}

		switch (Before->Type)
		{
		case EJson::Number:
			if (!FMath::IsNearlyEqual(Before->AsNumber(), After->AsNumber(), Tolerance))
			{
				OutChanges.Add(FString::Printf(TEXT("%s: %s -> %s"), *Path, *Describe(Before), *Describe(After)));
			}
			break;

		case EJson::String:
			if (!Before->AsString().Equals(After->AsString(), ESearchCase::CaseSensitive))
			{
				OutChanges.Add(FString::Printf(TEXT("%s: %s -> %s"), *Path, *Describe(Before), *Describe(After)));
			}
			break;

		case EJson::Boolean:
			if (Before->AsBool() != After->AsBool())
			{
				OutChanges.Add(FString::Printf(TEXT("%s: %s -> %s"), *Path, *Describe(Before), *Describe(After)));
			}
			break;

		case EJson::Array:
		{
			const TArray<TSharedPtr<FDataBridgeValue>>& A = Before->AsArray();
			const TArray<TSharedPtr<FDataBridgeValue>>& B = After->AsArray();
			if (A.Num() != B.Num())
			{
				OutChanges.Add(FString::Printf(TEXT("%s: %d -> %d items"), *Path, A.Num(), B.Num()));
			}
			const int32 Count = FMath::Max(A.Num(), B.Num());
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Collect(A.IsValidIndex(Index) ? A[Index] : nullptr, B.IsValidIndex(Index) ? B[Index] : nullptr,
					FString::Printf(TEXT("%s[%d]"), *Path, Index), OutChanges, Tolerance);
			}
			break;
		}

		case EJson::Object:
		{
			const TMap<FString, TSharedPtr<FDataBridgeValue>>& A = Before->AsObject()->Values;
			const TMap<FString, TSharedPtr<FDataBridgeValue>>& B = After->AsObject()->Values;
			for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : A)
			{
				Collect(Pair.Value, B.FindRef(Pair.Key), Path + TEXT(".") + Pair.Key, OutChanges, Tolerance);
			}
			for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : B)
			{
				if (!A.Contains(Pair.Key))
				{
					Collect(nullptr, Pair.Value, Path + TEXT(".") + Pair.Key, OutChanges, Tolerance);
				}
			}
			break;
		}

		default:
			break;
		}
	}
}