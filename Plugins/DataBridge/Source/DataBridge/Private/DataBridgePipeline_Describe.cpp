#include "DataBridgePipeline.h"

#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeSchemaBuilder.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Interfaces/DataBridgeHandler.h"
#include "UObject/Class.h"

TSharedPtr<FDataBridgeObject> FDataBridgePipeline::Describe(const UClass* Class, FDataBridgeContext& Ctx)
{
	using namespace DataBridge;

	if (!Class)
	{
		Ctx.AddError(TEXT("Class is null"));
		return nullptr;
	}

	const int32 ErrorsBefore = Ctx.GetErrorCount();

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
	return Schema.Build(Class);
}

