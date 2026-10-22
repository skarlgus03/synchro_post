#include "DataBridgePipeline.h"

#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeSchemaBuilder.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Interfaces/DataBridgeHandler.h"
#include "UObject/Class.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonValue.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

namespace DataBridgePipelineDescribe
{
	

	// 목록 하나당 상한 (설명서가 너무 커지지 않게)
	constexpr int32 MaxCatalogEntries = 300;

	TArray<TSharedPtr<FDataBridgeValue>> ToSortedArray(TArray<FString>& Values, const FString& What, FDataBridgeContext& Ctx)
	{
		Values.Sort();
		if (Values.Num() > MaxCatalogEntries)
		{
			Ctx.AddWarning(FString::Printf(TEXT("Catalog for '%s' has %d entries; only the first %d are listed"),
				*What, Values.Num(), MaxCatalogEntries));
			Values.SetNum(MaxCatalogEntries);
		}

		TArray<TSharedPtr<FDataBridgeValue>> Items;
		for (const FString& Value : Values)
		{
			Items.Add(MakeShared<FJsonValueString>(Value));
		}
		return Items;
	}

	// /Game 아래의 Roots 중 하나에 속하는지. 루트 자체도 포함
	bool IsUnderRoots(const FString& PackagePath, const TArray<FString>& Roots)
	{
		for (const FString& Root : Roots)
		{
			if (PackagePath == Root || PackagePath.StartsWith(Root + TEXT("/")))
			{
				return true;
			}
		}
		return false;
	}

	// 직접 등록한 태그 → { "태그": "설명" } (설명 = 태그 편집기의 Comment, 없으면 "")
	TSharedRef<FDataBridgeObject> CatalogTags()
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

		FGameplayTagContainer All;
		Manager.RequestAllGameplayTags(All, /*OnlyIncludeDictionaryTags*/ true);

		TArray<FGameplayTag> Tags;
		All.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });

		TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
		for (const FGameplayTag& Tag : Tags)
		{
			FString Comment;
			FName FirstSource;
			bool bExplicit = false, bRestricted = false, bAllowNonRestrictedChildren = false;
			Manager.GetTagEditorData(Tag.GetTagName(), Comment, FirstSource, bExplicit, bRestricted, bAllowNonRestrictedChildren);
			Out->SetStringField(Tag.ToString(), Comment);
		}
		return Out;
	}

	// asset 필드가 가리킨 클래스별 → /Game 아래의 실제 에셋 경로
	TSharedRef<FDataBridgeObject> CatalogAssets(const TArray<const UClass*>& Classes,const TArray<FString>& Roots, FDataBridgeContext& Ctx)
	{
		const IAssetRegistry& Registry = IAssetRegistry::GetChecked();
		TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();

		for (const UClass* Class : Classes)
		{
			// "아무 오브젝트나"인 필드는 프로젝트 전체가 나오므로 뺀다
			if (Class == UObject::StaticClass())
			{
				continue;
			}

			FARFilter Filter;
			Filter.ClassPaths.Add(Class->GetClassPathName());
			Filter.bRecursiveClasses = true;
			for (const FString& Root : Roots)
			{
				Filter.PackagePaths.Add(FName(*Root));
			}
			Filter.bRecursivePaths = true;
			Filter.bRecursivePaths = true;

			TArray<FAssetData> Found;
			Registry.GetAssets(Filter, Found);

			TArray<FString> Paths;
			for (const FAssetData& Asset : Found)
			{
				// Export와 같은 표기 (대표 에셋이면 /Game/A/DA_X)
				Paths.Add(FDataBridgeReferenceResolver::ToPreferredForm(Asset.GetObjectPathString()));
			}
			Out->SetArrayField(Class->GetPathName(), ToSortedArray(Paths, Class->GetPathName(), Ctx));
		}
		return Out;
	}

	// class 필드의 기준 클래스별 → 넣을 수 있는 클래스 경로 (로드되지 않은 BP 포함)
	TSharedRef<FDataBridgeObject> CatalogClasses(const TArray<const UClass*>& Bases,const TArray<FString>& Roots ,FDataBridgeContext& Ctx)
	{
		const IAssetRegistry& Registry = IAssetRegistry::GetChecked();
		TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();

		for (const UClass* Base : Bases)
		{
			if (Base == UObject::StaticClass())
			{
				continue;
			}

			TSet<FTopLevelAssetPath> Derived;
			Registry.GetDerivedClassNames({ Base->GetClassPathName() }, TSet<FTopLevelAssetPath>(), Derived);
			Derived.Add(Base->GetClassPathName());   // 기준 클래스 자신도 후보

			TArray<FString> Paths;
			for (const FTopLevelAssetPath& ClassPath : Derived)
			{
				// BP 컴파일 중 생기는 임시 클래스
				const FString Name = ClassPath.GetAssetName().ToString();
				if (Name.StartsWith(TEXT("SKEL_")) || Name.StartsWith(TEXT("REINST_")))
				{
					continue;
				}
				// BP 클래스는 범위 안의 것만. C++ 클래스(/Script/...)는 폴더가 없으므로 항상 포함
				const FString PackagePath = ClassPath.GetPackageName().ToString();
				if (!PackagePath.StartsWith(TEXT("/Script/")) && !IsUnderRoots(PackagePath, Roots))
				{
					continue;
				}
				// 로드된 클래스면 추상/폐기 여부를 확인할 수 있다 (로드 안 된 BP는 그대로 둔다)
				if (const UClass* Loaded = FindObject<UClass>(nullptr, *ClassPath.ToString()))
				{
					if (Loaded->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
					{
						continue;
					}
				}
				Paths.Add(ClassPath.ToString());
			}
			Out->SetArrayField(Base->GetPathName(), ToSortedArray(Paths, Base->GetPathName(), Ctx));
		}
		return Out;
	}

	TSharedRef<FDataBridgeObject> BuildCatalog(const FDataBridgeSchemaBuilder& Schema, const TArray<FString>& Roots, FDataBridgeContext& Ctx)
	{
		using namespace DataBridge;

		// 에디터 시작 직후에는 색인이 덜 찼을 수 있다
		if (IAssetRegistry::GetChecked().IsLoadingAssets())
		{
			Ctx.AddWarning(TEXT("Asset Registry is still scanning; catalog.assets and catalog.classes may be incomplete"));
		}

		TArray<TSharedPtr<FDataBridgeValue>> RootValues;
		for (const FString& Root : Roots)
		{
			RootValues.Add(MakeShared<FJsonValueString>(Root));
		}

		// 카탈로그: 루트/태그/에셋/클래스
		TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
		Out->SetArrayField(SchemaKeys::Roots, RootValues);
		Out->SetObjectField(SchemaKeys::Tags, CatalogTags());
		Out->SetObjectField(SchemaKeys::Assets, CatalogAssets(Schema.GetAssetClasses(), Roots, Ctx));
		Out->SetObjectField(SchemaKeys::Classes, CatalogClasses(Schema.GetClassBases(), Roots, Ctx));
		return Out;
	}

	
}


TSharedPtr<FDataBridgeObject> FDataBridgePipeline::Describe(const UClass* Class, FDataBridgeContext& Ctx,
	const TArray<FString>& CatalogRoots)
{
	using namespace DataBridge;
	
	const int32 ErrorsBefore = Ctx.GetErrorCount();

	if (!Class)
	{
		Ctx.AddError(TEXT("Class is null"));
		return nullptr;
	}

	// 카탈로그 범위: "/Game/SynchroPost/" → "/Game/SynchroPost". 비우면 /Game 전체
	TArray<FString> Roots;
	for (FString Root : CatalogRoots)
	{
		Root.RemoveFromEnd(TEXT("/"));
		if (!Root.StartsWith(TEXT("/")))
		{
			Ctx.AddError(FString::Printf(TEXT("Catalog root '%s' must be a content path such as /Game/Folder"), *Root));
			continue;
		}
		Roots.AddUnique(Root);
	}
	if (Roots.Num() == 0)
	{
		Roots.Add(TEXT("/Game"));
	}

	const IDataBridgeHandler* Handler = FDataBridgeRegistry::FindHandler(Class);
	if (!Handler)
	{
		Ctx.AddError(FString::Printf(TEXT("No handler for class '%s'"), *Class->GetPathName()));
		return nullptr;
	}

	// 1. 모양: 핸들러가 루트 클래스와, 따라 나오는 구조체/Instanced 클래스를 장부에 올린다
	FDataBridgeSchemaBuilder Schema;
	Handler->Describe(Class, Schema, Ctx);

	// 2. 기본값: 생략한 필드가 무엇이 되는지 (새 에셋의 최상위, Instanced 블록 모두 클래스 기본값)
	//    CDO를 기존 Export로 내보낸다. 문제가 있어도 설명서 자체는 유효하므로 경고 하나로만 남긴다
	const FDataBridgeReferenceResolver Resolver;
	for (const UClass* Described : Schema.GetClasses())
	{
		const TSharedPtr<FDataBridgeObject> Description = Schema.FindClass(Described);
		const IDataBridgeHandler* DescribedHandler = FDataBridgeRegistry::FindHandler(Described);
		if (!Description.IsValid() || !DescribedHandler)
		{
			continue;
		}

		FDataBridgeContext DefaultsCtx;
		const TSharedPtr<FDataBridgeObject> Defaults = DescribedHandler->Export(Described->GetDefaultObject(), Resolver, DefaultsCtx);
		if (Defaults.IsValid())
		{
			Description->SetObjectField(SchemaKeys::Defaults, Defaults);
		}
		if (DefaultsCtx.GetIssues().Num() > 0)
		{
			Ctx.AddWarning(FString::Printf(TEXT("Defaults of '%s' exported with issues:\n%s"),
				*Described->GetPathName(), *DefaultsCtx.BuildReport()));
		}
	}

	if (Ctx.GetErrorCount() > ErrorsBefore)
	{
		return nullptr;
	}
	// 3. 카탈로그: 설명서가 언급한 태그/에셋/클래스 중 실제로 존재하는 것
	const TSharedRef<FDataBridgeObject> Root = Schema.Build(Class);
	Root->SetObjectField(SchemaKeys::Catalog, DataBridgePipelineDescribe::BuildCatalog(Schema, Roots, Ctx));
	return Root;
}

