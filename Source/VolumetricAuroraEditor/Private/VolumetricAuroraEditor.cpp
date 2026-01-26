// Copyright (c) 2026 R&B. All rights reserved.

#include "VolumetricAuroraEditor.h"
#include "PropertyEditorModule.h"
#include "VolumetricAuroraDetailsCustomization.h"
#include "SDFBakerCustomization.h"
#include "VolumetricAurora.h"
#include "SplineSDFTextureBakerComponent.h"
#include "VolumetricAuroraStyle.h"

#define LOCTEXT_NAMESPACE "FVolumetricAuroraEditorModule"

bool FVolumetricAuroraEditorModule::bIsCreatingAssetWhileSaving = false;

void FVolumetricAuroraEditorModule::StartupModule()
{
	FVolumetricAuroraStyle::Initialize();

	// Get PropertyEditor module to register custom details panel
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register custom details panel for AVolumetricAurora
	// This replaces the default details panel with our customized version
	PropertyModule.RegisterCustomClassLayout(
		AVolumetricAurora::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FVolumetricAuroraDetailsCustomization::MakeInstance)
	);

	PropertyModule.RegisterCustomClassLayout(
		USplineSDFTextureBakerComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FSDFBakerCustomization::MakeInstance)
	);

	// Store class name for cleanup in ShutdownModule
	RegisteredCustomizations.Add(AVolumetricAurora::StaticClass()->GetFName());
	RegisteredCustomizations.Add(USplineSDFTextureBakerComponent::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("VolumetricAuroraEditor module started"));
}

void FVolumetricAuroraEditorModule::ShutdownModule()
{
	// Unregister all custom details panels when module unloads
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		for (const FName& ClassName : RegisteredCustomizations)
		{
			PropertyModule.UnregisterCustomClassLayout(ClassName);
		}
	}

	RegisteredCustomizations.Empty();

	FVolumetricAuroraStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

// Implements the module interface - required for Unreal to load this module
IMPLEMENT_MODULE(FVolumetricAuroraEditorModule, VolumetricAuroraEditor)

