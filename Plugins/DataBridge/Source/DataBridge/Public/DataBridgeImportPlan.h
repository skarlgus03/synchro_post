#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"

enum class EDataBridgeImportAction : uint8
{
	Create, // 그 경로에 에셋이 없다. -> 새로 만든다 
	Update, // 같은 클래스의 에셋이 이미 있다. -> 덮어씀
};


// 계획표의 한 줄 = 에셋 하나. UObject 포인터는 담지 않고 경로만 담는다.
struct DATABRIDGE_API  FDataBridgeImportEntry
{
	int32 SourceIndex = INDEX_NONE;          // 봉투 assets[] 안의 위치 (에러 경로용)
	FString PackageName;                     // /Game/Data/Unit/DA_Skeleton
	FString AssetName;                       // DA_Skeleton
	TWeakObjectPtr<UClass> Class;            // BP 클래스는 GC될 수 있으므로 Weak. Commit 때 재확인
	EDataBridgeImportAction Action = EDataBridgeImportAction::Create;
	TSharedPtr<FDataBridgeObject> Data;      // 이 에셋의 data IR

	FString GetObjectPath() const { return PackageName + TEXT(".") + AssetName; }
};

// "이 문서를 넣으면 무슨 일이 일어나는가". 만드는 동안 에셋은 건드리지 않는다.
struct DATABRIDGE_API FDataBridgeImportPlan
{
	TArray<FDataBridgeImportEntry> Entries;

	int32 Num(EDataBridgeImportAction Action) const;
	const FDataBridgeImportEntry* FindByPackageName(const FString& PackageName) const;

	// 사람이 읽는 요약 (로그, 테스트, 나중의 Dry-run 창)
	FString Describe() const;
};