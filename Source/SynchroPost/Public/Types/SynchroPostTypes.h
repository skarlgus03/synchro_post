#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SynchroPostTypes.generated.h"

UENUM(BlueprintType)
enum class EStatModType : uint8
{
	Flat UMETA(DisplayName = "Flat"),
	Percent UMETA(DisplayName = "Percent")
};

USTRUCT(BlueprintType)
struct FStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Modifier")
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Modifier")
	EStatModType ModType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Modifier")
	float ModifierValue;
};

USTRUCT(BlueprintType)
struct FStatDetailed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float UnitBaseValue = 0.0f;


	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float SlotFlatBonus = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float SlotPercentBonus = 0.0f;


	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float ItemFlatBonus = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float ItemPercentBonus = 0.0f;


	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float BuffFlatBonus = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float BuffPercentBonus = 0.0f;


	UPROPERTY(BlueprintReadOnly, Category = "Stat Detail")
	float FinalValue = 0.0f;

	void UpdateFinalValue()
	{
		float TotalFlat = UnitBaseValue + SlotFlatBonus + ItemFlatBonus + BuffFlatBonus;

		float TotalPercent = SlotPercentBonus + ItemPercentBonus + BuffPercentBonus;

		FinalValue = TotalFlat * (1.0f + TotalPercent);

		FinalValue = FMath::Max(0.0f, TotalFlat + TotalPercent);
	}
};


UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
	Self UMETA(DisplayName = "Self"),
	Ally UMETA(DisplayName = "Ally"),
	Enemy UMETA(DisplayName = "Enemy"),
	Area UMETA(DisplayName = "Area")
};

UENUM(BlueprintType)
enum class ESkillShapeType : uint8
{
	None UMETA(DisplayName = "None"),
	Square UMETA(DisplayName = "Square"),
	Circle UMETA(DisplayName = "Circle"),
	Line UMETA(DisplayName = "Line"),
	Cross UMETA(DisplayName = "Cross")
};

USTRUCT(BlueprintType)
struct FSquareShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Width = 3;

	UPROPERTY(EditAnywhere)
	int32 Height = 3;
};

USTRUCT(BlueprintType)
struct FCircleShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Radius = 2;

	UPROPERTY(EditAnywhere)
	float Angle = 360.0f;
};

USTRUCT(BlueprintType)
struct FLineShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Length = 3;

	UPROPERTY(EditAnywhere)
	int32 Width = 1;
};

USTRUCT(BlueprintType)
struct FCrossShapeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 ArmLength = 3;
};

USTRUCT(BlueprintType)
struct FSkillTargetingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	ESkillTargetType TargetType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	ESkillShapeType ShapeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting | Square", meta = (EditCondition = "ShapeType == ESkillShapeType::Square"))
	FSquareShapeData SquareData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting | Circle", meta = (EditCondition = "ShapeType == ESkillShapeType::Circle"))
	FCircleShapeData CircleData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting | Line", meta = (EditCondition = "ShapeType == ESkillShapeType::Line"))
	FLineShapeData LineData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting | Cross", meta = (EditCondition = "ShapeType == ESkillShapeType::Cross"))
	FCrossShapeData CrossData;

};


UENUM(BlueprintType)
enum class EUnitTeam : uint8
{
	Player UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy"),
	Neutral UMETA(DisplayName = "Neutral")
};