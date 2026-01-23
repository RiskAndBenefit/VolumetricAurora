// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @brief Editor module for VolumetricAurora plugin
 * 
 * Provides editor-only functionality:
 * - Details panel customizations
 * - Custom editor modes
 * - Asset factories and utilities
 */
class FVolumetricAuroraEditorModule : public IModuleInterface
{
public:
    /**
     * @brief Called when module is loaded
     * Registers custom details panel customizations
     */
    virtual void StartupModule() override;

    /**
     * @brief Called when module is unloaded
     * Unregisters all customizations
     */
    virtual void ShutdownModule() override;

	static bool bIsCreatingAssetWhileSaving;

private:
    /** Store registered customization names for cleanup */
    TArray<FName> RegisteredCustomizations;
};