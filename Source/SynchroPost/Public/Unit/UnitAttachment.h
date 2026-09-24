#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnitAttachment.generated.h"

class AUnit;
class UMeshComponent;
class USkeletalMesh;
class UStaticMesh;

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class SYNCHROPOST_API UUnitAttachment : public UObject
{
	GENERATED_BODY()
	

public: 

	UMeshComponent* CreateAttachedComponent(AUnit* Owner) const;
protected:

	virtual UMeshComponent* NewMeshComponent(AUnit* Owner) const;

	UPROPERTY(EditAnywhere)
	FName AttachSocketName;

	UPROPERTY(EditAnywhere)
	FTransform RelativeTransform;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SYNCHROPOST_API USkeletalMeshAttachment : public UUnitAttachment
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
	
	virtual UMeshComponent* NewMeshComponent(AUnit* Owner) const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SYNCHROPOST_API UStaticMeshAttachment : public UUnitAttachment
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	virtual UMeshComponent* NewMeshComponent(AUnit* Owner) const override;
};
