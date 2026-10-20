#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
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

UCLASS(Abstract, HideDropdown)
class UDataBridgeTestAbstractAsset : public UObject
{
	GENERATED_BODY()
};

UENUM()
enum class EDataBridgeTestEnum : uint8
{
	Alpha,
	Beta,
	Gamma,
};

USTRUCT()
struct FDataBridgeTestStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Test")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, Category = "Test")
	FString Label;
};

// 테스트 전용. 워커가 다뤄야 하는 타입을 한 클래스에 모은다.
// 총합본임
UCLASS(HideDropdown)
class UDataBridgeTestAllTypes : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Test") bool bFlag = false;
	UPROPERTY(EditAnywhere, Category = "Test") int32 Int = 0;
	UPROPERTY(EditAnywhere, Category = "Test") int64 BigInt = 0;
	UPROPERTY(EditAnywhere, Category = "Test") float Float = 0.f;
	UPROPERTY(EditAnywhere, Category = "Test") double Double = 0.0;
	UPROPERTY(EditAnywhere, Category = "Test") uint8 Byte = 0;
	UPROPERTY(EditAnywhere, Category = "Test") FString Str;
	UPROPERTY(EditAnywhere, Category = "Test") FName Name;
	UPROPERTY(EditAnywhere, Category = "Test") FText Text;
	UPROPERTY(EditAnywhere, Category = "Test") EDataBridgeTestEnum Enum = EDataBridgeTestEnum::Alpha;

	UPROPERTY(EditAnywhere, Category = "Test") FVector Vector = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Test") FRotator Rotator = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, Category = "Test") FTransform Transform;
	UPROPERTY(EditAnywhere, Category = "Test") FDataBridgeTestStruct Struct;

	UPROPERTY(EditAnywhere, Category = "Test") FGameplayTag Tag;
	UPROPERTY(EditAnywhere, Category = "Test") FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, Category = "Test") TArray<int32> IntArray;
	UPROPERTY(EditAnywhere, Category = "Test") TArray<FDataBridgeTestStruct> StructArray;
	UPROPERTY(EditAnywhere, Category = "Test") TSet<FName> NameSet;
	UPROPERTY(EditAnywhere, Category = "Test") TMap<FName, int32> NameToInt;
	UPROPERTY(EditAnywhere, Category = "Test") TMap<FGameplayTag, int32> TagToInt;
	UPROPERTY(EditAnywhere, Category = "Test") TMap<int32, FString> IntToString;

	UPROPERTY(EditAnywhere, Category = "Test") TObjectPtr<UObject> HardRef;
	UPROPERTY(EditAnywhere, Category = "Test") TSoftObjectPtr<UObject> SoftRef;
	UPROPERTY(EditAnywhere, Category = "Test") TSubclassOf<UObject> ClassRef;
	UPROPERTY(EditAnywhere, Category = "Test") TSoftClassPtr<UObject> SoftClassRef;

	UPROPERTY(EditAnywhere, Instanced, Category = "Test") TObjectPtr<UDataBridgeTestChild> SingleChild;
	UPROPERTY(EditAnywhere, Instanced, Category = "Test") TArray<TObjectPtr<UDataBridgeTestChild>> Children;

	// 아래 셋은 내보내면 안 된다
	UPROPERTY(Transient, EditAnywhere, Category = "Test") int32 TransientValue = 0;
	UPROPERTY() int32 NotEditable = 0;
	UPROPERTY(VisibleAnywhere, Category = "Test") int32 ReadOnly = 0;
};