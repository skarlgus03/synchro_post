#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Unit/UnitStatDataAsset.h"
#include "Skill/SkillDataAsset.h"
#include "UnitDataAsset.generated.h"

class UUnitPresentationBase;
class UUnitHealthBarWidget;
class UUnitAnimSetDataAsset;

UENUM(BlueprintType)
enum class EHealthBarDisplay : uint8
{
	/*떠다니는 체력바를 사용하지 않음.*/
	Disabled UMETA(DisplayName = "사용 안함"),

	/*피격/호버/선택 시에만 보임. */
	EventOnly UMETA(DisplayName = "이벤트 시에만"),
	
	/*체력이 최대가 아니면 계속 보임*/
	WhenDamaged UMETA(DisplayName = "피해를 입었을 때"),

	/*항상 보임*/
	Always UMETA(DisplayName = "항상 표시"),
};

UCLASS()
class SYNCHROPOST_API UUnitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<USkeletalMesh> UnitMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UUnitAnimSetDataAsset> AnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UUnitStatDataAsset> UnitStatData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills", meta = (Categories = "Skill.Slot"))
	TMap<FGameplayTag, TObjectPtr<USkillDataAsset>> SkillDataAssetMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gimmick Stats", meta = (Categories = "Stat.Gimmick"))
	TMap<FGameplayTag, int32> GimmickStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSubclassOf<UUnitPresentationBase> PresentationClass;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUnitHealthBarWidget> HealthBarWidgetClass;

	// 체력바 높이의 기준이 될 본/소켓 이름. 비워두면 메시 바운즈를 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FName HealthBarSocket = NAME_None;

	// 위에서 구한 기준 높이에 더할 여유값. 음수가능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	float HealthBarZOffset = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	EHealthBarDisplay HealthBarDisplay = EHealthBarDisplay::WhenDamaged;
};
