#include "DataBridgeTypes.h"

DEFINE_LOG_CATEGORY(LogDataBridge);

FString FDataBridgeIssue::ToString() const
{
	// Severity에 따라 "Error" 또는 "Warning" 레이블을 선택
	const TCHAR* Label = (Severity == EDataBridgeSeverity::Error) ? TEXT("Error") : TEXT("Warning");
	// Path가 비어있으면 "[Label] Message" 형식, 그렇지 않으면 "[Label] Path: Message" 형식으로 반환
	return Path.IsEmpty()
		? FString::Printf(TEXT("[%s] %s"), Label, *Message)
		: FString::Printf(TEXT("[%s] %s: %s"), Label, *Path, *Message);
}



void FDataBridgeContext::PushKey(const FString& Key)
{
	Segments.Add(DataBridge::MakeKeySegment(Key));
}

void FDataBridgeContext::PushIndex(int32 Index)
{
	Segments.Add(FString::Printf(TEXT("[%d]"), Index));
}

void FDataBridgeContext::Pop()
{
	if (ensureMsgf(Segments.Num() > 0, TEXT("DataBridge: path stack underflow")))
	{
		Segments.Pop(EAllowShrinking::No);
	}
}

FString FDataBridgeContext::GetPath() const
{
	FString Result;
	for (const FString& Segment : Segments)
	{
		Result += Segment;
	}
	Result.RemoveFromStart(TEXT("."));
	return Result;
}

void FDataBridgeContext::AddError(const FString& Message)
{
	AddIssue(EDataBridgeSeverity::Error, Message);
}

void FDataBridgeContext::AddWarning(const FString& Message)
{
	AddIssue(EDataBridgeSeverity::Warning, Message);
}

void FDataBridgeContext::AddIssue(EDataBridgeSeverity Severity, const FString& Message)
{
	FDataBridgeIssue& Issue = Issues.AddDefaulted_GetRef();
	Issue.Severity = Severity;
	Issue.Path = GetPath();
	Issue.Message = Message;

	if (Severity == EDataBridgeSeverity::Error)
	{
		++ErrorCount;
	}
}

FString FDataBridgeContext::BuildReport() const
{
	FString Report;
	for (const FDataBridgeIssue& Issue : Issues)
	{
		Report += Issue.ToString();
		Report += LINE_TERMINATOR;
	}
	return Report;
}

DATABRIDGE_API FString DataBridge::MakeKeySegment(const FString& Key)
{
	// 키에 경로 구분 문자 ('.', '[', ']')가 포함되어 있으면, 키를 ["Key"] 형식으로 감싸서 반환
	const bool bNeedsQuote = Key.Contains(TEXT(".")) || Key.Contains(TEXT("[")) || Key.Contains(TEXT("]"));
	return bNeedsQuote
		? FString::Printf(TEXT("[\"%s\"]"), *Key)
		: TEXT(".") + Key;
}
