#include "DataBridgeSchemaBuilder.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "UObject/Class.h"
#include "DataBridgeSchemaBuilder.h"

bool FDataBridgeSchemaBuilder::TryBeginClass(const UClass* Class)
{
	// 아직 설명 안했으면 예약하고 true
	const FString Path = Class->GetPathName();
	if (Classes.Contains(Path))
	{
		return false;
	}
	Classes.Add(Path, nullptr);
	ClassOrder.Add(Class);
	return true;
}

void FDataBridgeSchemaBuilder::SetClass(const UClass* Class, const TSharedRef<FDataBridgeObject>& Description)
{
	Classes.Add(Class->GetPathName(), Description);
}

TSharedPtr<FDataBridgeObject> FDataBridgeSchemaBuilder::FindClass(const UClass* Class) const
{
	return Classes.FindRef(Class->GetPathName());
}

bool FDataBridgeSchemaBuilder::TryBeginStruct(const UScriptStruct* Struct)
{
	const FString Path = Struct->GetPathName();
	if (Structs.Contains(Path))
	{
		return false;
	}
	Structs.Add(Path, nullptr);
	return true;
}

void FDataBridgeSchemaBuilder::SetStruct(const UScriptStruct* Struct, const TSharedRef<FDataBridgeObject>& Description)
{
	Structs.Add(Struct->GetPathName(), Description);
}

void FDataBridgeSchemaBuilder::AddAssetClass(const UClass* Class)
{
	AssetClasses.AddUnique(Class);
}

void FDataBridgeSchemaBuilder::AddClassBase(const UClass* Class)
{
	ClassBases.AddUnique(Class);
}

void FDataBridgeSchemaBuilder::AddUndocumented(const FString& Where)
{
	Undocumented.AddUnique(Where);
}

TSharedRef<FDataBridgeObject> FDataBridgeSchemaBuilder::Build(const UClass* Root) const
{
	using namespace DataBridge;

	// Table에 Source를 순회하며 옮겨적음.
	const auto MakeTable = [](const TMap<FString, TSharedPtr<FDataBridgeObject>>& Source)
		{
			TSharedRef<FDataBridgeObject> Table = MakeShared<FDataBridgeObject>();
			for (const TPair<FString, TSharedPtr<FDataBridgeObject>>& Pair : Source)
			{
				if (Pair.Value.IsValid())
				{
					Table->SetObjectField(Pair.Key, Pair.Value);
				}
			}
			return Table;
		};

	TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	Out->SetStringField(Keys::Format, SchemaFormatVersion);
	Out->SetStringField(SchemaKeys::Root, Root ? Root->GetPathName() : FString());
	Out->SetObjectField(SchemaKeys::Classes, MakeTable(Classes));
	Out->SetObjectField(SchemaKeys::Structs, MakeTable(Structs));

	TArray<TSharedPtr<FDataBridgeValue>> UndocumentedValues;
	for (const FString& Where : Undocumented)
	{
		UndocumentedValues.Add(MakeShared<FJsonValueString>(Where));
	}
	Out->SetArrayField(SchemaKeys::Undocumented, UndocumentedValues);

	return Out;
}
