#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/SPSkillStructure.h"
#include "SkillComponent.generated.h"

class USkillBase;
class USkillDataAsset;
class UUnitDataAsset;
class AUnit;
class UStateComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillComponent();

	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	// Initialize the skill component with the provided unit data asset
	void InitializeSkillComponent(UUnitDataAsset* UnitDataAsset);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	USkillBase* FindSkillByTag(const FGameplayTag& SkillSlotTag) const;

	const TArray<FSkillEntry>& GetSkillEntries() const { return SkillList.Entries; }


	// Check if the skill can be executed.
	bool CanExecuteSkill(const FGameplayTag& SkillSlotTag) const;


	FORCEINLINE const FSkillResource* FindResource(const FGameplayTag& ResourceTag) const
	{
		return CurrentResources.FindByPredicate(
			[&ResourceTag](const FSkillResource& Res)
			{
				return Res.ResourceTag == ResourceTag;
			});
	}

	FORCEINLINE FSkillResource* FindResource(const FGameplayTag& ResourceTag)
	{
		return CurrentResources.FindByPredicate(
			[&ResourceTag](FSkillResource& Res)
			{
				return Res.ResourceTag == ResourceTag;
			});
	}

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	UPROPERTY(Replicated)
	FSkillList SkillList;

	UPROPERTY()
	TObjectPtr<AUnit> OwnerUnit;

	UPROPERTY()
	TObjectPtr<UStateComponent> CachedStateComponent;

	// Resource management
	UPROPERTY(ReplicatedUsing = OnRep_CurrentResources)
	TArray<FSkillResource> CurrentResources;

	UFUNCTION()
	void OnRep_CurrentResources();

protected:

	bool HasEnoughResource(const FGameplayTag& ResourceTag, int32 RequireValue) const;

	bool CheckCommonState() const;

	FGameplayTagContainer GetSiblingStateTag() const;

	FSkillExecutionContext BuildExecutionContext() const;
};
