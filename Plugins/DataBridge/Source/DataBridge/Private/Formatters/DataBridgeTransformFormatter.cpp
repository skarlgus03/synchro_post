#include "Formatters/DataBridgeTransformFormatter.h"

#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	// 소수 4자리 반올림 + -0 정리
	double Clean(double Value)
	{
		const double Rounded = FMath::RoundToDouble(Value * 10000.0) / 10000.0;
		return Rounded == 0.0 ? 0.0 : Rounded;
	}

	TSharedRef<FDataBridgeObject> MakeComponents(const TCHAR* A, double VA, const TCHAR* B, double VB, const TCHAR* C, double VC)
	{
		const TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
		Out->SetNumberField(A, Clean(VA));
		Out->SetNumberField(B, Clean(VB));
		Out->SetNumberField(C, Clean(VC));
		return Out;
	}

	// { "A": n, "B": n, "C": n } 중 적힌 것만 덮어쓴다. 모르는 키나 숫자가 아닌 값은 에러.
	bool ReadComponents(const FDataBridgeValue& Value, const TCHAR* A, double& VA, const TCHAR* B, double& VB, const TCHAR* C, double& VC, FDataBridgeContext& Ctx)
	{
		const TSharedPtr<FDataBridgeObject>* Object = nullptr;
		if (!Value.TryGetObject(Object))
		{
			Ctx.AddError(FString::Printf(TEXT("Expected { \"%s\", \"%s\", \"%s\" }"), A, B, C));
			return false;
		}

		bool bOk = true;
		for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : (*Object)->Values)
		{
			FDataBridgePathScope Scope(Ctx, Pair.Key);
			double* Target = Pair.Key == A ? &VA : Pair.Key == B ? &VB : Pair.Key == C ? &VC : nullptr;
			if (!Target)
			{
				Ctx.AddError(FString::Printf(TEXT("Unknown component. Use %s, %s or %s"), A, B, C));
				bOk = false;
				continue;
			}
			if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Number)
			{
				Ctx.AddError(TEXT("Expected number"));
				bOk = false;
				continue;
			}
			*Target = Pair.Value->AsNumber();
		}
		return bOk;
	}
}

UScriptStruct* FDataBridgeTransformFormatter::GetSupportedStruct() const
{
	return TBaseStructure<FTransform>::Get();
}

TSharedPtr<FDataBridgeValue> FDataBridgeTransformFormatter::Export(const void* StructData, FDataBridgeContext& Ctx) const
{
	const FTransform& Transform = *static_cast<const FTransform*>(StructData);
	const FVector Location = Transform.GetLocation();
	const FRotator Rotation = Transform.Rotator();
	const FVector Scale = Transform.GetScale3D();

	const TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	Out->SetObjectField(TEXT("Location"), MakeComponents(TEXT("X"), Location.X, TEXT("Y"), Location.Y, TEXT("Z"), Location.Z));
	Out->SetObjectField(TEXT("Rotation"), MakeComponents(TEXT("Pitch"), Rotation.Pitch, TEXT("Yaw"), Rotation.Yaw, TEXT("Roll"), Rotation.Roll));
	Out->SetObjectField(TEXT("Scale"), MakeComponents(TEXT("X"), Scale.X, TEXT("Y"), Scale.Y, TEXT("Z"), Scale.Z));
	return MakeShared<FJsonValueObject>(Out);
}

bool FDataBridgeTransformFormatter::Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const
{
	const TSharedPtr<FDataBridgeObject>* Object = nullptr;
	if (!Value.TryGetObject(Object))
	{
		Ctx.AddError(TEXT("Expected { \"Location\", \"Rotation\", \"Scale\" }"));
		return false;
	}

	FTransform& Transform = *static_cast<FTransform*>(StructData);
	FVector Location = Transform.GetLocation();
	FRotator Rotation = Transform.Rotator();
	FVector Scale = Transform.GetScale3D();

	bool bOk = true;
	for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : (*Object)->Values)
	{
		FDataBridgePathScope Scope(Ctx, Pair.Key);
		if (!Pair.Value.IsValid())
		{
			Ctx.AddError(TEXT("Value is missing"));
			bOk = false;
		}
		else if (Pair.Key == TEXT("Location"))
		{
			bOk &= ReadComponents(*Pair.Value, TEXT("X"), Location.X, TEXT("Y"), Location.Y, TEXT("Z"), Location.Z, Ctx);
		}
		else if (Pair.Key == TEXT("Rotation"))
		{
			bOk &= ReadComponents(*Pair.Value, TEXT("Pitch"), Rotation.Pitch, TEXT("Yaw"), Rotation.Yaw, TEXT("Roll"), Rotation.Roll, Ctx);
		}
		else if (Pair.Key == TEXT("Scale"))
		{
			bOk &= ReadComponents(*Pair.Value, TEXT("X"), Scale.X, TEXT("Y"), Scale.Y, TEXT("Z"), Scale.Z, Ctx);
		}
		else
		{
			Ctx.AddError(TEXT("Unknown part. Use Location, Rotation or Scale"));
			bOk = false;
		}
	}

	if (bOk)
	{
		Transform = FTransform(Rotation, Location, Scale);
	}
	return bOk;
}