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