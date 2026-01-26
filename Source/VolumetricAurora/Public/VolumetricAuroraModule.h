// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FVolumetricAuroraModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
