#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DataBridgeTestTypes.generated.h"

// 테스트 전용. Effect 하나를 흉내 낸다.
UCLASS(EditInlineNew, DefaultToInstanced, HideDropdown)
class UDataBridgeTestChild : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Test")
	int32 Value = 0;
};

// 테스트 전용. Instanced 자식을 가진 에셋(스킬 DA + Effects)을 흉내 낸다.
UCLASS(HideDropdown)
class UDataBridgeTestAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Test")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, Instanced, Category = "Test")
	TArray<TObjectPtr<UDataBridgeTestChild>> Children;
};