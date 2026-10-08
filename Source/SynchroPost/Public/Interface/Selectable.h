

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Selectable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class USelectable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 커서가 올라감(호버) / 선택됨에 반응하는 대상.
 * 정보 조회용으로 사용된다.
 */
class SYNCHROPOST_API ISelectable
{
	GENERATED_BODY()

	
public:
	
	virtual void SetHovered(bool bHovered) = 0;
	virtual void SetSelected(bool bSelected) = 0;
};
