#pragma once
#include "Modules/ModuleManager.h"

class FVolumetricAuroraShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};