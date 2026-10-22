#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

class UClass;
class UScripStruct;


namespace DataBridge
{
	inline constexpr const TCHAR* SchemaFormatVersion = TEXT("databridge-describe/1");

	// 설명서 키
	namespace SchemaKeys
	{
		inline constexpr const TCHAR* Root = TEXT("root");
		inline constexpr const TCHAR* Classes = TEXT("classes");
		inline constexpr const TCHAR* Structs = TEXT("structs");
		inline constexpr const TCHAR* Fields = TEXT("fields");
		inline constexpr const TCHAR* Defaults = TEXT("defaults");
		inline constexpr const TCHAR* Catalog = TEXT("catalog");
		inline constexpr const TCHAR* Undocumented = TEXT("undocumented");
		inline constexpr const TCHAR* Size = TEXT("size");				  // C 스타일 sizeof
		inline constexpr const TCHAR* Type = TEXT("type");
		inline constexpr const TCHAR* Description = TEXT("description");  // 필드 주석 (ToolTip)
		inline constexpr const TCHAR* Note = TEXT("note");                // 포매터가 붙이는 사용법
		inline constexpr const TCHAR* Values = TEXT("values");            // enum / flags 이름
		inline constexpr const TCHAR* Items = TEXT("items");              // array / set 원소 타입
		inline constexpr const TCHAR* Key = TEXT("key");                  // map 키 타입
		inline constexpr const TCHAR* Value = TEXT("value");              // map 값 타입
		inline constexpr const TCHAR* MapForm = TEXT("form");             // "object" | "pairs"
		inline constexpr const TCHAR* Allowed = TEXT("allowed");          // instanced에 넣을 수 있는 $class
		inline constexpr const TCHAR* Class = TEXT("class");              // asset / class 참조의 기준 클래스
		inline constexpr const TCHAR* Struct = TEXT("struct");            // struct 타입의 정의 이름
		inline constexpr const TCHAR* Categories = TEXT("categories");    // 태그 허용 접두사
		inline constexpr const TCHAR* Min = TEXT("min");
		inline constexpr const TCHAR* Max = TEXT("max");
		inline constexpr const TCHAR* Shape = TEXT("shape");              // 포매터의 객체 모양
		inline constexpr const TCHAR* Tags = TEXT("tags");                // catalog.tags
		inline constexpr const TCHAR* Assets = TEXT("assets");            // catalog.assets
		inline constexpr const TCHAR* Roots = TEXT("roots");              // catalog.roots
	}

	// 설명서의 type 값
	namespace SchemaTypes
	{
		inline constexpr const TCHAR* Bool = TEXT("bool");
		inline constexpr const TCHAR* Int = TEXT("int");
		inline constexpr const TCHAR* Float = TEXT("float");
		inline constexpr const TCHAR* String = TEXT("string");
		inline constexpr const TCHAR* Name = TEXT("name");
		inline constexpr const TCHAR* Text = TEXT("text");
		inline constexpr const TCHAR* Enum = TEXT("enum");
		inline constexpr const TCHAR* Flags = TEXT("flags");
		inline constexpr const TCHAR* Struct = TEXT("struct");
		inline constexpr const TCHAR* Array = TEXT("array");
		inline constexpr const TCHAR* Set = TEXT("set");
		inline constexpr const TCHAR* Map = TEXT("map");
		inline constexpr const TCHAR* Asset = TEXT("asset");
		inline constexpr const TCHAR* Class = TEXT("class");
		inline constexpr const TCHAR* Instanced = TEXT("instanced");
		inline constexpr const TCHAR* Tag = TEXT("tag");
		inline constexpr const TCHAR* Tags = TEXT("tags");
		inline constexpr const TCHAR* Transform = TEXT("transform");
	}
}

// 설명서 한 부를 만드는 동안 쓰는 공용 장부. Pipeline이 만들고 핸들러/워커가 채운다.
// 클래스와 구조체는 경로 이름으로 한 번씩만 적는다 (정의 테이블).
// UClass/UScriptStruct 포인터를 들고 있지만 Describe 한 번(동기 호출) 동안만 살아 있으므로 GC 걱정 없음.
class DATABRIDGE_API FDataBridgeSchemaBuilder : public FNoncopyable
{
public:
	// 아직 설명하지 않은 클래스면 자리를 예약하고 true. 이미 했거나 하는 중이면 false.
	// 설명하기 전에 먼저 예약하므로 순환 참조에서도 무한 반복하지 않는다.
	bool TryBeginClass(const UClass* Class);
	void SetClass(const UClass* Class, const TSharedRef<FDataBridgeObject>& Description);
	TSharedPtr<FDataBridgeObject> FindClass(const UClass* Class) const;

	// 아직 설명하지 않은 구조체면 자리를 예약하고 true. 
	bool TryBeginStruct(const UScriptStruct* Struct);
	void SetStruct(const UScriptStruct* Struct, const TSharedRef<FDataBridgeObject>& Description);

	// 카탈로그에 실제 에셋 목록을 올릴 클래스 (asset 필드가 가리키는 클래스. 실제 에셋)
	void AddAssetClass(const UClass* Class);
	const TArray<const UClass*>& GetAssetClasses() const { return AssetClasses; }

	// 카탈로그에 실제 클래스 목록을 올릴 기준 클래스 (class 필드가 가리키는 MetaClass. 설계도)
	void AddClassBase(const UClass* Class);
	const TArray<const UClass*>& GetClassBases() const { return ClassBases; }

	// 설명된 클래스 (예약 순서). Pipeline이 defaults를 붙일 때 쓴다
	const TArray<const UClass*>& GetClasses() const { return ClassOrder; }

	// 주석(ToolTip)이 없는 필드. 예: "SkillDataAsset.SkillCost"
	void AddUndocumented(const FString& Where);

	// 설명서 본문 (catalog는 Pipeline이 붙인다)
	TSharedRef<FDataBridgeObject> Build(const UClass* Root) const;

private:
	TMap<FString, TSharedPtr<FDataBridgeObject>> Classes;   // 경로 → 설명 (nullptr = 예약만 됨)
	TArray<const UClass*> ClassOrder;
	TMap<FString, TSharedPtr<FDataBridgeObject>> Structs;
	TArray<const UClass*> AssetClasses;
	TArray<FString> Undocumented;
	TArray<const UClass*> ClassBases;
};