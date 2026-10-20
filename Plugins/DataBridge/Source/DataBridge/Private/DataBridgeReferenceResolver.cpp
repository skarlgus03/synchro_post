#include "DataBridgeReferenceResolver.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "DataBridgeTypes.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectIterator.h"

FString FDataBridgeReferenceResolver::NormalizeObjectPath(const FString& Path)
{
	const FString Trimmed = Path.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || Trimmed.Contains(TEXT(".")))
	{
		return Trimmed;
	}
	return Trimmed + TEXT(".") + FPackageName::GetShortName(Trimmed);
}

FString FDataBridgeReferenceResolver::ToPreferredForm(const FString& ObjectPath)
{
	int32 DotIndex = INDEX_NONE;
	if (!ObjectPath.FindChar(TEXT('.'), DotIndex) || ObjectPath.Contains(TEXT(":")))
	{
		return ObjectPath;
	}

	const FString PackageName = ObjectPath.Left(DotIndex);
	const FString ObjectName = ObjectPath.Mid(DotIndex + 1);
	return ObjectName == FPackageName::GetShortName(PackageName) ? PackageName : ObjectPath;
}

void FDataBridgeReferenceResolver::Register(const FString& LogicalPath, UObject* Object)
{
	check(Object);
	const FString ObjectPath = NormalizeObjectPath(LogicalPath);
	ObjectByPath.Add(ObjectPath, Object);
	PathByObject.Add(Object, ToPreferredForm(ObjectPath));
}

UObject* FDataBridgeReferenceResolver::ResolveHard(const FString& Path, const UClass* ExpectedClass, FDataBridgeContext& Ctx) const
{
	const FString ObjectPath = NormalizeObjectPath(Path);

	UObject* Object = nullptr;
	if (UObject* const* Registered = ObjectByPath.Find(ObjectPath))
	{
		Object = *Registered;   // 이 문서 안의 대상 → 연습이면 복제본, 실전이면 진짜
	}
	else
	{
		// 문서 밖의 기존 대상 → 메모리에 있으면 그대로, 없으면 로드 (읽기만, 수정 아님)
		Object = FSoftObjectPath(ObjectPath).ResolveObject();
		if (!Object)
		{
			Object = LoadObject<UObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}

	if (!Object)
	{
		Ctx.AddError(FString::Printf(TEXT("Referenced object '%s' not found"), *Path));
		return nullptr;
	}
	if (ExpectedClass && !Object->IsA(ExpectedClass))
	{
		Ctx.AddError(FString::Printf(TEXT("'%s' is a '%s', but '%s' is expected"),
			*Path, *Object->GetClass()->GetName(), *ExpectedClass->GetName()));
		return nullptr;
	}
	return Object;
}

bool FDataBridgeReferenceResolver::CheckSoft(const FString& Path, FDataBridgeContext& Ctx) const
{
	const FString ObjectPath = NormalizeObjectPath(Path);

	if (ObjectByPath.Contains(ObjectPath))
	{
		return true;   // 같은 문서에서 만들 예정인 대상
	}

	const FSoftObjectPath SoftPath(ObjectPath);
	if (SoftPath.ResolveObject() || IAssetRegistry::GetChecked().GetAssetByObjectPath(SoftPath).IsValid())
	{
		return true;
	}

	Ctx.AddError(FString::Printf(TEXT("Soft reference target '%s' does not exist"), *Path));
	return false;
}

FString FDataBridgeReferenceResolver::ToLogicalPath(const UObject* Object) const
{
	if (!Object)
	{
		return FString();
	}
	if (const FString* Logical = PathByObject.Find(Object))
	{
		return *Logical;   // 복제본 → 원래 논리 경로
	}
	return ToPreferredForm(Object->GetPathName());
}

UClass* FDataBridgeReferenceResolver::FindClass(const FString& ClassText, FDataBridgeContext& Ctx)
{
	const FString Text = ClassText.TrimStartAndEnd();

	// 없으면 에러
	if (Text.IsEmpty())
	{
		Ctx.AddError(TEXT("Class name is empty"));
		return nullptr;
	}

	if (Text.StartsWith(TEXT("/")))
	{
		UClass* Class = FindObject<UClass>(nullptr, *Text);
		if (!Class)
		{
			// 블루프린트 클래스는 로드해야 찾을 수 있다
			Class = LoadObject<UClass>(nullptr, *Text, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		if (!Class)
		{
			Ctx.AddError(FString::Printf(TEXT("Class '%s' not found"), *Text));
		}
		return Class;
	}

	// 이름만 있으면 여러 개일 수 있으므로 모두 찾아본다 구버전은 제외
	TArray<UClass*> Matches;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->GetName() == Text && !It->HasAnyClassFlags(CLASS_NewerVersionExists))
		{
			Matches.Add(*It);
		}
	}

	if (Matches.Num() == 0)
	{
		Ctx.AddError(FString::Printf(TEXT("Class '%s' not found. Check the name or use the full path (/Script/Module.Class"), *Text));
		return nullptr;
	}

	// 2개 이상이면, 에러 메세지를 만들어서 후보를 보여준다
	if (Matches.Num() > 1)
	{
		TArray<FString> Candidates;
		for (const UClass* Match : Matches)
		{
			Candidates.Add(Match->GetPathName());
		}
		Ctx.AddError(FString::Printf(TEXT("Class name '%s' is ambiguous. Use one of: %s"), *Text, *FString::Join(Candidates, TEXT(", "))));
		return nullptr;
	}

	return Matches[0];
}
