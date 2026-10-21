#include "Formatters/DataBridgeGameplayTagFormatters.h"

#include "DataBridgeTypes.h"
#include "Dom/JsonValue.h"
#include "GameplayTagContainer.h"
#include "DataBridgeSchemaBuilder.h"

namespace
{
	// 문자열 → 등록된 태그. 빈 문자열은 빈 태그. 없는 태그면 에러.
	bool ParseTag(const FString& Text, FGameplayTag& OutTag, FDataBridgeContext& Ctx)
	{
		const FString Trimmed = Text.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			OutTag = FGameplayTag();
			return true;
		}

		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Trimmed), /*ErrorIfNotFound*/ false);
		if (!Tag.IsValid())
		{
			Ctx.AddError(FString::Printf(TEXT("Unknown gameplay tag '%s'"), *Trimmed));
			return false;
		}
		OutTag = Tag;
		return true;
	}
}


// ---- FGameplayTag ----

UScriptStruct* FDataBridgeGameplayTagFormatter::GetSupportedStruct() const
{
	return FGameplayTag::StaticStruct();
}

TSharedPtr<FDataBridgeValue> FDataBridgeGameplayTagFormatter::Export(const void* StructData, FDataBridgeContext& Ctx) const
{
	const FGameplayTag& Tag = *static_cast<const FGameplayTag*>(StructData);
	return MakeShared<FJsonValueString>(Tag.IsValid() ? Tag.ToString() : FString());
}

bool FDataBridgeGameplayTagFormatter::Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const
{
	FGameplayTag Tag;
	if (Value.Type == EJson::Null)
	{
		// null = 빈 태그
	}
	else if (Value.Type != EJson::String)
	{
		Ctx.AddError(TEXT("Expected a tag string such as \"Stat.Attack\""));
		return false;
	}
	else if (!ParseTag(Value.AsString(), Tag, Ctx))
	{
		return false;
	}

	*static_cast<FGameplayTag*>(StructData) = Tag;
	return true;
}

// ---- FGameplayTagContainer ----

UScriptStruct* FDataBridgeGameplayTagContainerFormatter::GetSupportedStruct() const
{
	return FGameplayTagContainer::StaticStruct();
}

TSharedPtr<FDataBridgeValue> FDataBridgeGameplayTagContainerFormatter::Export(const void* StructData, FDataBridgeContext& Ctx) const
{
	const FGameplayTagContainer& Container = *static_cast<const FGameplayTagContainer*>(StructData);

	TArray<TSharedPtr<FDataBridgeValue>> Items;
	for (const FGameplayTag& Tag : Container)
	{
		Items.Add(MakeShared<FJsonValueString>(Tag.ToString()));
	}
	return MakeShared<FJsonValueArray>(Items);
}

bool FDataBridgeGameplayTagContainerFormatter::Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const
{
	FGameplayTagContainer Container;

	if (Value.Type == EJson::Null)
	{
		// null = 빈 컨테이너
	}
	else if (Value.Type != EJson::Array)
	{
		Ctx.AddError(TEXT("Expected an array of tag strings such as [\"Trait.Fire\"]"));
		return false;
	}
	else
	{
		bool bAllValid = true;
		const TArray<TSharedPtr<FDataBridgeValue>>& Items = Value.AsArray();
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			FDataBridgePathScope Scope(Ctx, Index);

			FGameplayTag Tag;
			if (!Items[Index].IsValid() || Items[Index]->Type != EJson::String)
			{
				Ctx.AddError(TEXT("Expected a tag string"));
				bAllValid = false;
			}
			else if (!ParseTag(Items[Index]->AsString(), Tag, Ctx))
			{
				bAllValid = false;
			}
			else if (Tag.IsValid())
			{
				Container.AddTag(Tag);
			}
		}
		if (!bAllValid)
		{
			return false;
		}
	}

	*static_cast<FGameplayTagContainer*>(StructData) = Container;
	return true;
}

TSharedRef<FDataBridgeObject> FDataBridgeGameplayTagFormatter::DescribeValue() const
{
	using namespace DataBridge;
	TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	Out->SetStringField(SchemaKeys::Type, SchemaTypes::Tag);
	Out->SetStringField(SchemaKeys::Note, TEXT("Gameplay tag name such as \"A.B.C\". Empty string means no tag. Must be a registered tag (see catalog.tags)."));
	return Out;
}

TSharedRef<FDataBridgeObject> FDataBridgeGameplayTagContainerFormatter::DescribeValue() const
{
	using namespace DataBridge;
	TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	Out->SetStringField(SchemaKeys::Type, SchemaTypes::Tags);
	Out->SetStringField(SchemaKeys::Note, TEXT("Array of gameplay tag names. Each must be a registered tag (see catalog.tags)."));
	return Out;
}