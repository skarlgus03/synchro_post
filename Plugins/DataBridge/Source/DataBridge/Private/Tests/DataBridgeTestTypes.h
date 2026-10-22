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

// 고를 수 있는 Instanced 자식. HideDropDown은 상속되지 않는다 (CLASS_Inherit에 없음).
// Describe의 allowed 필터 검증용: 이건 allowed에 들어가고, HideDropDown인 부모는 빠져야 한다
UCLASS(EditInlineNew)
class UDataBridgeTestPickableChild : public UDataBridgeTestChild
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Test")
	int32 Extra = 0;
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

// 값 = 비트 (프로젝트의 ESkillTargetFlags와 같은 방식). 2번 비트(4)는 일부러 비워 둔다
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDataBridgeTestMaskFlags : uint8
{
	None = 0 UMETA(Hidden),
	Red = 1 << 0,
	Green = 1 << 1,
	Blue = 1 << 3,
};

// 값 = 비트 번호 (엔진 기본 방식). First → 1, Second → 2, Third → 4
UENUM(meta = (Bitflags))
enum class EDataBridgeTestBitIndexFlags : uint8
{
	First,
	Second,
	Third,
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
	UPROPERTY(EditAnywhere, Category = "Test") TSubclassOf<UDataBridgeTestChild> ChildClassRef;

	UPROPERTY(EditAnywhere, Instanced, Category = "Test") TObjectPtr<UDataBridgeTestChild> SingleChild;
	UPROPERTY(EditAnywhere, Instanced, Category = "Test") TArray<TObjectPtr<UDataBridgeTestChild>> Children;

	UPROPERTY(EditAnywhere, Category = "Test", meta = (Bitmask, BitmaskEnum = "/Script/DataBridge.EDataBridgeTestMaskFlags"))
	int32 MaskFlags = 0;

	UPROPERTY(EditAnywhere, Category = "Test", meta = (Bitmask, BitmaskEnum = "/Script/DataBridge.EDataBridgeTestBitIndexFlags"))
	int32 BitIndexFlags = 0;

	/**
	 * Documented test field.
	 * Second line.
	 */
	UPROPERTY(EditAnywhere, Category = "Test", meta = (ClampMin = "0", ClampMax = "10"))
	int32 ClampedInt = 0;

	// 아래 셋은 내보내면 안 된다
	UPROPERTY(Transient, EditAnywhere, Category = "Test") int32 TransientValue = 0;
	UPROPERTY() int32 NotEditable = 0;
	UPROPERTY(VisibleAnywhere, Category = "Test") int32 ReadOnly = 0;
};