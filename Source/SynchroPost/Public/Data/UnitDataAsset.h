// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/UnitStatDataAsset.h"
#include "UnitDataAsset.generated.h"

UCLASS()
class SYNCHROPOST_API UUnitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<USkeletalMesh> UnitMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UUnitStatDataAsset> UnitStatData;
};
