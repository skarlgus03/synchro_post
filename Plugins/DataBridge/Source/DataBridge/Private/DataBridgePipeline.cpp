#include "DataBridgePipeline.h"

#include "DataBridgeImportPlan.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectIterator.h"



// ---------------------------------------------------------------------------
// BuildImportPlan 내부
// ---------------------------------------------------------------------------

namespace DataBridgePipelinePrivate
{
	// 문서 검사를 통과한 항목 (아직 프로젝트와 대조하기 전)
	struct FEnvelopeEntry
	{
		int32 Index = INDEX_NONE;
		FString PackageName;
		FString AssetName;
		FString ClassText;
		TSharedPtr<FDataBridgeObject> Data;
	};

	const TCHAR* JsonTypeName(EJson Type)
	{
		switch (Type)
		{
		case EJson::Null:    return TEXT("null");
		case EJson::String:  return TEXT("string");
		case EJson::Number:  return TEXT("number");
		case EJson::Boolean: return TEXT("boolean");
		case EJson::Array:   return TEXT("array");
		case EJson::Object:  return TEXT("object");
		default:             return TEXT("none");
		}
	}

	// Key 필드가 있고 기대 타입인지 확인. 실패하면 Key 경로로 에러를 남기고 nullptr.
	TSharedPtr<FDataBridgeValue> GetRequiredField(const FDataBridgeObject& Obj, const TCHAR* Key, EJson ExpectedType, FDataBridgeContext& Ctx)
	{
		FDataBridgePathScope Scope(Ctx, Key);

		TSharedPtr<FDataBridgeValue> Value = Obj.TryGetField(Key);
		if (!Value.IsValid()) // 필드 없으면 에러
		{
			Ctx.AddError(TEXT("Missing required field"));
			return nullptr;
		}
		if (Value->Type != ExpectedType) // 기대 타입이 아니라면 에러
		{
			Ctx.AddError(FString::Printf(TEXT("Expected %s, got %s"), JsonTypeName(ExpectedType), JsonTypeName(Value->Type)));
			return nullptr;
		}
		return Value;
	}

	// "/Game/A/DA_X" 또는 "/Game/A/DA_X.DA_X" → 패키지 이름 + 에셋 이름
	bool ParseAssetPath(const FString& Text, FString& OutPackageName, FString& OutAssetName, FDataBridgeContext& Ctx)
	{

		// 점을 찾으면 점을 기준으로 패키지 이름과 에셋 이름을 나눈다.
		FString PackageName = Text;
		FString ObjectName;
		int32 DotIndex = INDEX_NONE;
		if (Text.FindChar(TEXT('.'), DotIndex))
		{
			PackageName = Text.Left(DotIndex);
			ObjectName = Text.Mid(DotIndex + 1);
		}

		// /Temp가 읽기 전용 마운트라서 읽기 전용 경로도 형식상 허용하고, 금지 영역은 아래에서 직접 막는다.
		// 언리얼 패키지 경로 규칙에 맞는지 확인함. 
		FText Reason;
		if (!FPackageName::IsValidLongPackageName(PackageName, /*bIncludeReadOnlyRoots*/ true, &Reason))
		{
			Ctx.AddError(FString::Printf(TEXT("Invalid package path '%s': %s"), *PackageName, *Reason.ToString()));
			return false;
		}

		// 이 도구에서 쓰기를 금지한 영역을 막는다. /Script, /Memory, /Engine, /Config
		if (FPackageName::IsScriptPackage(PackageName)
			|| FPackageName::IsMemoryPackage(PackageName)
			|| PackageName.StartsWith(TEXT("/Engine/"))
			|| PackageName.StartsWith(TEXT("/Config/")))
		{
			Ctx.AddError(FString::Printf(TEXT("Writing under '%s' is not allowed. Use a content path such as /Game/..."), *PackageName));
			return false;
		}

		// 마지막 이름과 점 뒤 이름이 다르면 에러. (패키지 이름과 에셋 이름이 다르면 안 됨)
		const FString ShortName = FPackageName::GetShortName(PackageName);
		if (!ObjectName.IsEmpty() && ObjectName != ShortName)
		{
			Ctx.AddError(FString::Printf(TEXT("Asset name '%s' must match the package name '%s'"), *ObjectName, *ShortName));
			return false;
		}

		OutPackageName = PackageName;
		OutAssetName = ShortName;
		return true;
	}

	// 검사 1: 문서만 본다. 통과한 항목을 OutEntries에 담는다.
	// 더 진행할 수 없는 구조 오류(assets 없음 등)면 false.
	bool ValidateEnvelope(const FDataBridgeObject& Root, TArray<FEnvelopeEntry>& OutEntries, FDataBridgeContext& Ctx)
	{
		using namespace DataBridge;

		if (const TSharedPtr<FDataBridgeValue> Format = GetRequiredField(Root, Keys::Format, EJson::String, Ctx))
		{
			// 포맷 버전이 맞는지 확인
			if (Format->AsString() != FormatVersion)
			{
				FDataBridgePathScope Scope(Ctx, Keys::Format);
				Ctx.AddError(FString::Printf(TEXT("Unsupported format version. Expected '%s', got '%s'"), FormatVersion, *Format->AsString()));
			}
		}

		// assets이 없거나 배열이 아니면 false 로 종료
		const TSharedPtr<FDataBridgeValue> AssetsValue = GetRequiredField(Root, Keys::Assets, EJson::Array, Ctx);
		if (!AssetsValue.IsValid())
		{
			return false;
		}
		const TArray<TSharedPtr<FDataBridgeValue>>& Assets = AssetsValue->AsArray();
		FDataBridgePathScope AssetsScope(Ctx, Keys::Assets);

		if (Assets.Num() == 0)
		{
			Ctx.AddWarning(TEXT("Nothing to import"));
		}

		TMap<FString, int32> FirstIndexByPackage; // FString 키는 대소문자 무시 (패키지 이름 규칙과 같음)

		for (int32 Index = 0; Index < Assets.Num(); ++Index)
		{
			FDataBridgePathScope IndexScope(Ctx, Index);

			// 배열 요소가 오브젝트인지 확인 아니면 오류를 기록하고 continue
			const TSharedPtr<FDataBridgeValue>& Item = Assets[Index];
			if (!Item.IsValid() || Item->Type != EJson::Object)
			{
				Ctx.AddError(FString::Printf(TEXT("Expected object, got %s"), Item.IsValid() ? JsonTypeName(Item->Type) : TEXT("none")));
				continue;
			}
			const FDataBridgeObject& Obj = *Item->AsObject();

			FEnvelopeEntry Entry;
			Entry.Index = Index;
			bool bValid = true;

			// path를 검사하고 패키지 이름과 에셋 이름을 얻는다. 중복이면 에러.
			if (const TSharedPtr<FDataBridgeValue> PathValue = GetRequiredField(Obj, Keys::Path, EJson::String, Ctx))
			{
				FDataBridgePathScope PathScope(Ctx, Keys::Path);
				if (!ParseAssetPath(PathValue->AsString(), Entry.PackageName, Entry.AssetName, Ctx))
				{
					bValid = false;
				}
				else if (const int32* FirstIndex = FirstIndexByPackage.Find(Entry.PackageName))
				{
					Ctx.AddError(FString::Printf(TEXT("Duplicate path. Already used by assets[%d]"), *FirstIndex));
					bValid = false;
				}
				else
				{
					FirstIndexByPackage.Add(Entry.PackageName, Index);
				}
			}
			else
			{
				bValid = false;
			}

			// class를 검사하고 유효한지 확인
			if (const TSharedPtr<FDataBridgeValue> ClassValue = GetRequiredField(Obj, Keys::Class, EJson::String, Ctx))
			{
				// class 가 문자열인지 확인하고 앞뒤 공백을 제거해서 저장한다.
				Entry.ClassText = ClassValue->AsString().TrimStartAndEnd();
				if (Entry.ClassText.IsEmpty()) // 공백 제거 후 빈 문자열이면 에러
				{
					FDataBridgePathScope ClassScope(Ctx, Keys::Class);
					Ctx.AddError(TEXT("Class must not be empty"));
					bValid = false;
				}
			}
			else
			{
				bValid = false;
			}

			// data가 객체 형태인지 확인하고 저장한다. 아니면 에러.
			if (const TSharedPtr<FDataBridgeValue> DataValue = GetRequiredField(Obj, Keys::Data, EJson::Object, Ctx))
			{
				Entry.Data = DataValue->AsObject();
			}
			else
			{
				bValid = false;
			}

			if (bValid)
			{
				OutEntries.Add(MoveTemp(Entry));
			}
		}
		return true;
	}

	// 전체 경로(/Script/Module.Class, /Game/BP_X.BP_X_C) 또는 짧은 이름(UnitDataAsset)
	// 클래스 문자열을 보고 UClass*를 찾는다. 없으면 에러 기록 후 nullptr 반환.
	UClass* ResolveClass(const FString& ClassText, FDataBridgeContext& Ctx)
	{
		UClass* Class = nullptr;

		if (ClassText.StartsWith(TEXT("/")))
		{
			Class = FindObject<UClass>(nullptr, *ClassText);
			if (!Class)
			{
				// 블루프린트 클래스는 로드해야 찾을 수 있다 (로드일 뿐 에셋 수정은 아님)
				Class = LoadObject<UClass>(nullptr, *ClassText, nullptr, LOAD_NoWarn | LOAD_Quiet);
			}
			if (!Class)
			{
				Ctx.AddError(FString::Printf(TEXT("Class '%s' not found"), *ClassText));
				return nullptr;
			}
		}
		else
		{
			TArray<UClass*> Matches;
			for (TObjectIterator<UClass> It; It; ++It)
			{
				if (It->GetName() == ClassText && !It->HasAnyClassFlags(CLASS_NewerVersionExists))
				{
					Matches.Add(*It);
				}
			}

			if (Matches.Num() == 0)
			{
				Ctx.AddError(FString::Printf(TEXT("Class '%s' not found. Check the name or use the full path (/Script/Module.Class)"), *ClassText));
				return nullptr;
			}
			if (Matches.Num() > 1)
			{
				TArray<FString> Candidates;
				for (const UClass* Match : Matches)
				{
					Candidates.Add(Match->GetPathName());
				}
				Ctx.AddError(FString::Printf(TEXT("Class name '%s' is ambiguous. Use one of: %s"), *ClassText, *FString::Join(Candidates, TEXT(", "))));
				return nullptr;
			}
			Class = Matches[0];
		}

		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			Ctx.AddError(FString::Printf(TEXT("Class '%s' is abstract and cannot be instantiated"), *Class->GetPathName()));
			return nullptr;
		}
		if (Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			Ctx.AddError(FString::Printf(TEXT("Class '%s' is deprecated"), *Class->GetPathName()));
			return nullptr;
		}
		if (Class->IsChildOf(AActor::StaticClass()))
		{
			Ctx.AddError(FString::Printf(TEXT("Actor classes cannot be imported as standalone assets ('%s')"), *Class->GetPathName()));
			return nullptr;
		}
		return Class;
	}

	// 검사 2: 대상 경로의 현재 상태로 Create/Update를 정한다. 에셋을 로드하거나 바꾸지 않는다.
	bool ResolveAction(const FEnvelopeEntry& Entry, const UClass* Class, IAssetRegistry& AssetRegistry,
		EDataBridgeImportAction& OutAction, FDataBridgeContext& Ctx)
	{
		using namespace DataBridge;

		const FSoftObjectPath ObjectPath(Entry.PackageName + TEXT(".") + Entry.AssetName);

		// 기존 에셋이 있으면 클래스가 같은지 확인하고, Redirector면 에러. 없으면 Create.
		auto CheckExisting = [&](const FTopLevelAssetPath& ExistingClassPath, bool bIsRedirector) -> bool
			{
				if (bIsRedirector)
				{
					FDataBridgePathScope Scope(Ctx, Keys::Path);
					Ctx.AddError(TEXT("Path points to a redirector. Use the redirector's target path or fix up redirectors first"));
					return false;
				}
				if (ExistingClassPath != Class->GetClassPathName())
				{
					FDataBridgePathScope Scope(Ctx, Keys::Class);
					Ctx.AddError(FString::Printf(TEXT("Existing asset is '%s' but the document says '%s'. Changing an asset's class is not supported"),
						*ExistingClassPath.ToString(), *Class->GetClassPathName().ToString()));
					return false;
				}
				OutAction = EDataBridgeImportAction::Update;
				return true;
			};

		// 에셋을 찾는다.

		// 1) 이미 메모리에 있는 오브젝트 (저장 전 새 에셋 포함). ResolveObject는 로드하지 않는다.
		if (const UObject* InMemory = ObjectPath.ResolveObject())
		{
			return CheckExisting(
				InMemory->GetClass()->GetClassPathName(),
				InMemory->IsA<UObjectRedirector>());
		}

		// 2) AssetRegistry (디스크의 에셋)
		const FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(ObjectPath);
		if (AssetData.IsValid())
		{
			return CheckExisting(AssetData.AssetClassPath, AssetData.IsRedirector());
		}

		// 3) 패키지는 있는데 그 이름의 에셋이 없음 → 덮어쓰면 안 됨
		TArray<FAssetData> AssetsInPackage;
		AssetRegistry.GetAssetsByPackageName(FName(*Entry.PackageName), AssetsInPackage);
		if (AssetsInPackage.Num() > 0 || FPackageName::DoesPackageExist(Entry.PackageName))
		{
			FDataBridgePathScope Scope(Ctx, Keys::Path);
			Ctx.AddError(FString::Printf(TEXT("Package '%s' already exists but has no asset named '%s'"), *Entry.PackageName, *Entry.AssetName));
			return false;
		}

		// 모두 없으면 새로 만들기로 결정한다
		OutAction = EDataBridgeImportAction::Create;
		return true;
	}
}

// ---------------------------------------------------------------------------
// FDataBridgePipeline::BuildImportPlan
// ---------------------------------------------------------------------------

bool FDataBridgePipeline::BuildImportPlan(const FDataBridgeObject& Root, FDataBridgeImportPlan& OutPlan, FDataBridgeContext& Ctx)
{
	using namespace DataBridgePipelinePrivate;
	using namespace DataBridge;

	OutPlan.Entries.Reset();
	const int32 ErrorsBefore = Ctx.GetErrorCount();

	// 1. 문서 자체 검사
	TArray<FEnvelopeEntry> Envelope;
	if (!ValidateEnvelope(Root, Envelope, Ctx))
	{
		return false;
	}

	// 2. 프로젝트와 대조 (읽기만)
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	if (AssetRegistry.IsLoadingAssets())
	{
		// 스캔 중에는 "없음"을 잘못 판단해 기존 에셋을 Create로 볼 수 있다
		Ctx.AddError(TEXT("Asset registry is still scanning. Try again after it finishes"));
		return false;
	}

	for (const FEnvelopeEntry& Entry : Envelope)
	{
		FDataBridgePathScope AssetsScope(Ctx, Keys::Assets);
		FDataBridgePathScope IndexScope(Ctx, Entry.Index);

		UClass* Class = nullptr;
		{
			FDataBridgePathScope ClassScope(Ctx, Keys::Class);
			Class = ResolveClass(Entry.ClassText, Ctx);
		}
		if (!Class)
		{
			continue;
		}

		EDataBridgeImportAction Action = EDataBridgeImportAction::Create;
		if (!ResolveAction(Entry, Class, AssetRegistry, Action, Ctx))
		{
			continue;
		}

		FDataBridgeImportEntry& PlanEntry = OutPlan.Entries.AddDefaulted_GetRef();
		PlanEntry.SourceIndex = Entry.Index;
		PlanEntry.PackageName = Entry.PackageName;
		PlanEntry.AssetName = Entry.AssetName;
		PlanEntry.Class = Class;
		PlanEntry.Action = Action;
		PlanEntry.Data = Entry.Data;
	}

	// 에러를 끝까지 모은 뒤, 하나라도 새로 생겼으면 계획 전체를 무효로 한다
	if (Ctx.GetErrorCount() > ErrorsBefore)
	{
		OutPlan.Entries.Reset();
		return false;
	}
	return true;
}