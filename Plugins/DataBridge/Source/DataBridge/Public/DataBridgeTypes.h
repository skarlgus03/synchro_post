#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class FJsonValue;

// IR 그릇. 엔진 Json 모듈의 DOM타입을 빌려 쓰지만 의미는
// 포맷과 무관한 중간표현이다. 
using FDataBridgeValue = FJsonValue;
using FDataBridgeObject = FJsonObject;

DATABRIDGE_API DECLARE_LOG_CATEGORY_EXTERN(LogDataBridge, Log, All);

namespace DataBridge
{
	// IR 계약. 포맷/핸들러는 문자열을 직접 쓰지 말고 이 상수를 사용해야함.
	inline constexpr const TCHAR* FormatVersion = TEXT("databridge/1");

	// 인스턴스화된 객체를 재귀적으로 만들 때 최대 깊이. 32 이상이면 스택 오버플로우 위험
	inline constexpr int32 MaxInstancedDepth = 32;

	// IR 경로의 키 한 칸. 경로 구분문자 (. [] ) 가 있으면, ["..."] 로 감싼다. 예:  ".Value", "[\"Stat.Attack\"]"
	DATABRIDGE_API FString MakeKeySegment(const FString& Key);

	namespace Keys
	{
		inline constexpr const TCHAR* Format = TEXT("format");
		inline constexpr const TCHAR* Assets = TEXT("assets");
		inline constexpr const TCHAR* Path = TEXT("path");
		inline constexpr const TCHAR* Class = TEXT("class");
		inline constexpr const TCHAR* Data = TEXT("data");
		inline constexpr const TCHAR* InstancedClass = TEXT("$class");
		
		// 맵
		inline constexpr const TCHAR* MapKey = TEXT("key");
		inline constexpr const TCHAR* MapValue = TEXT("value");
	}
}

enum class EDataBridgeSeverity : uint8
{
	Warning,
	Error,
};

struct DATABRIDGE_API FDataBridgeIssue
{
	EDataBridgeSeverity Severity = EDataBridgeSeverity::Error;
	FString Path;	// 예: assets[0].data.Effects[1].Parts[0].DamageType
	FString Message;

	// 이슈를 한 줄로 표현한 문자열. 로그/커맨드렛/AI 피드백용
	FString ToString() const;
};


// 한 번의 Export/Import 작업 동안 IR 경로와 이슈를 모은다.
class DATABRIDGE_API FDataBridgeContext
{
public:
	// 경로를 쌓고 뺄 수 있다. 경로는 "."과 "[]"로 구분된 문자열로 표현된다.
	void PushKey(const FString& Key);
	// 인덱스 문자열을 넣음 [] 로 감싼다. 예: [3]
	void PushIndex(int32 Index);
	// Segments 에서 마지막 한칸 뺀다.
	void Pop();

	// Segments에 저장된 문자열을 순서대로 합친다. 맨 앞의 "."은 제거한다. 예: assets[0].data.Effects[1].Parts[0].DamageType
	FString GetPath() const;

	void AddError(const FString& Message);
	void AddWarning(const FString& Message);

	bool HasErrors() const { return ErrorCount > 0; }
	int32 GetErrorCount() const { return ErrorCount; }
	const TArray<FDataBridgeIssue>& GetIssues() const { return Issues; }

	// 이슈 전체를 한 줄씩 이어 붙인 보고서 (로그/커맨드렛/AI 피드백용)
	FString BuildReport() const;

private:
	void AddIssue(EDataBridgeSeverity Severity, const FString& Message);


	TArray<FString> Segments; // ".Key" / "[3]" / "[\"Stat.Attack\"]"
	TArray<FDataBridgeIssue> Issues;
	int32 ErrorCount = 0;
};

// 스코프 동안 경로 한 칸을 쌓고, 벗어나면 자동으로 뺀다.
class FDataBridgePathScope
{
public:
	FDataBridgePathScope(FDataBridgeContext& InContext, const FString& Key)
		: Context(InContext) {
		Context.PushKey(Key);
	}

	FDataBridgePathScope(FDataBridgeContext& InContext, int32 Index)
		: Context(InContext) {
		Context.PushIndex(Index);
	}

	~FDataBridgePathScope() { Context.Pop(); }

	FDataBridgePathScope(const FDataBridgePathScope&) = delete;
	FDataBridgePathScope& operator=(const FDataBridgePathScope&) = delete;

private:
	FDataBridgeContext& Context;
};