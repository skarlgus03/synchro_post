#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Unit/UnitStatDataAsset.h"
#include "SynchroPostSettings.generated.h"



UCLASS(Config=Game, defaultconfig, meta=(DisplayName="SynchroPost Settings"))
class SYNCHROPOST_API USynchroPostSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:

	UPROPERTY(Config, EditAnywhere, Category = "Stats")
	TSoftObjectPtr<UUnitStatDataAsset> BaseStatDefaultData;
};
