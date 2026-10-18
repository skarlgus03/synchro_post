#pragma once

#include "Modules/ModuleManager.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Interfaces/DataBridgeHandler.h"

class FDataBridgeModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// 코어가 기본 제공하는 구현체. 외부 확장 모듈과 똑같이 창구를 통해 등록한다.
	TUniquePtr<IDataBridgeFormat> JsonFormat;
	TUniquePtr<IDataBridgeHandler> ReflectionHandler;
};
