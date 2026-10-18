#include "DataBridgeReferenceResolver.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "DataBridgeTypes.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"

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
