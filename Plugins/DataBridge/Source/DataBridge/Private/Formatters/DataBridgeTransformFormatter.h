#pragma once

#include "Interfaces/DataBridgeValueFormatter.h"

// FTransform ⇄ { "Location": {X,Y,Z}, "Rotation": {Pitch,Yaw,Roll}, "Scale": {X,Y,Z} }
// 엔진은 회전을 쿼터니언으로 저장하지만, 사람과 AI가 쓰기 쉬운 오일러 각(도)으로 바꾼다.
// Export 값은 소수 4자리로 반올림한다 (쿼터니언 변환 잡음 제거). Import는 적힌 축만 바꾼다.
class FDataBridgeTransformFormatter : public IDataBridgeValueFormatter
{
public:
	virtual UScriptStruct* GetSupportedStruct() const override;
	virtual TSharedPtr<FDataBridgeValue> Export(const void* StructData, FDataBridgeContext& Ctx) const override;
	virtual bool Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const override;
	virtual TSharedRef<FDataBridgeObject> DescribeValue() const override;
};