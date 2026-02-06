// Copyright (c) 2026 R&B. All rights reserved.

#include "VolumetricAuroraEditor.h"
#include "PropertyEditorModule.h"
#include "Customizations/VolumetricAuroraDetailsCustomization.h"
#include "Customizations/DFBakerCustomization.h"
#include "Customizations/AuroraFlowElementCustomization.h"
#include "Actors/VolumetricAurora.h"
#include "Components/SplineDFTextureBakerComponent.h"
#include "Types/AuroraFlowElement.h"
#include "Style/VolumetricAuroraStyle.h"

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
		USplineDFTextureBakerComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FDFBakerCustomization::MakeInstance)
	);

	// Register custom property type layout for FAuroraFlowElement struct
	// This customization adds a collapsible "Advanced" section to each array element
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FAuroraFlowElement::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FAuroraFlowElementCustomization::MakeInstance)
	);

	// Store class name for cleanup in ShutdownModule
	RegisteredCustomizations.Add(AVolumetricAurora::StaticClass()->GetFName());
	RegisteredCustomizations.Add(USplineDFTextureBakerComponent::StaticClass()->GetFName());
	RegisteredCustomizations.Add(FAuroraFlowElement::StaticStruct()->GetFName());

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
			PropertyModule.UnregisterCustomPropertyTypeLayout(ClassName);
		}
	}

	RegisteredCustomizations.Empty();

	FVolumetricAuroraStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

// Implements the module interface - required for Unreal to load this module
IMPLEMENT_MODULE(FVolumetricAuroraEditorModule, VolumetricAuroraEditor)

