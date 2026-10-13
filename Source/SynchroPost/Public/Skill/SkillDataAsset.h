
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPSkillStructure.h"
#include "SkillDataAsset.generated.h"

class USkillBase;
class USkillPresentation;
class USkillEffect;

/*
* 스킬 하나의 정의. 데이터+로직+연출 대본
*/
UCLASS()
class SYNCHROPOST_API USkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// 이름, 비용, 타게팅, 효과 숯치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Data")
	FSkillData SkillData;

	// 스킬의 로직을 구현하는 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Data")
	TSubclassOf<USkillBase> SkillLogicClass;

	// 무엇을 하는지, 위에서부터 차례대로 실행됨.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Effect")
	TArray<TObjectPtr<USkillEffect>> Effects;

	// 스킬 연출 대본
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Presentation")
	TObjectPtr<USkillPresentation> Presentation;
};
