#pragma once

#include "CoreMinimal.h"
#include "SPUIStructure.generated.h"

class UUnitHealthBarWidget;

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

// 머리 위 체력바 설정. 유닛, 장애물 공통. 소유자가 자기 DA에서 채운다.
USTRUCT(BlueprintType)
struct FHealthBarSettings
{
	GENERATED_BODY()

	// 비우면 프로젝트 기본 위젯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	TSubclassOf<UUnitHealthBarWidget> WidgetClass;

	// 높이 기준 본/소켓. 비우면 메시 바운즈 최상단
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	FName Socket = NAME_None;

	// 기준 높이에 더할 여유값. 음수 가능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	float ZOffset = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	EHealthBarDisplay Display = EHealthBarDisplay::WhenDamaged;
};
