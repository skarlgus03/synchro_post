

#pragma once

#include "CoreMinimal.h"

class FDataBridgeContext;
class UClass;
class UObject;

// 문서 속 "주소 글자"와 실제 오브젝트를 잇는 주소록.
// 파이프라인이 단계마다 새로 만들어 채운다 (연습 = 복제본, 실전 = 진짜).
// 오브젝트 수명은 파이프라인이 책임지고, 이 객체는 작업 한 번 동안만 산다.
class DATABRIDGE_API FDataBridgeReferenceResolver
{
public:
	// "/Game/A/DA_X" → "/Game/A/DA_X.DA_X". 이미 오브젝트 경로면 그대로.
	static FString NormalizeObjectPath(const FString& Path);

	// "/Game/A/DA_X.DA_X" → "/Game/A/DA_X" (대표 에셋만). 클래스나 서브오브젝트 경로는 그대로.
	static FString ToPreferredForm(const FString& ObjectPath);

	// 파이프라인 전용: 이 논리 경로는 지금 이 오브젝트를 가리킨다
	void Register(const FString& LogicalPath, UObject* Object);

	// 하드 참조: 주소록 → 메모리 → 로드 순으로 찾는다.
	// 없거나 ExpectedClass가 아니면 에러를 남기고 nullptr. (빈 참조 처리는 호출자 몫)
	UObject* ResolveHard(const FString& Path, const UClass* ExpectedClass, FDataBridgeContext& Ctx) const;

	// Soft 참조: 로드하지 않고 존재만 확인한다 (주소록 → 메모리 → AssetRegistry).
	bool CheckSoft(const FString& Path, FDataBridgeContext& Ctx) const;

	// Export용 역방향: 등록된 오브젝트면 논리 경로, 아니면 실제 경로 (선호 표기로)
	FString ToLogicalPath(const UObject* Object) const;

	// 클래스 이름 -> UClass. 전체 경로 또는 짧은 이름
	// 없거나 짧은 이름이 여러 클래스와겹치면 에러 + nullptr
	static UClass* FindClass(const FString& ClassText, FDataBridgeContext& Ctx);

private:
	TMap<FString, UObject*> ObjectByPath;        // 키: 정규화된 오브젝트 경로 (FString 키라 대소문자 무시)
	TMap<const UObject*, FString> PathByObject;  // 값: 선호 표기의 논리 경로
};