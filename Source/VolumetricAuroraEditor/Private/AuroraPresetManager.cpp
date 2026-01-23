// Fill out your copyright notice in the Description page of Project Settings.


#include "AuroraPresetManager.h"
#include "AuroraPresetAsset.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#include "VolumetricAuroraEditor.h"
#include "Interfaces/IPluginManager.h"

void UAuroraPresetManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString PresetDirName = TEXT("AuroraPresets");
	
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"));

	if (Plugin.IsValid())
	{
		PluginName = Plugin->GetName();
	}
	
	if (Plugin.IsValid())
	{
		PresetDiskPath = FPaths::Combine(Plugin->GetContentDir(), PresetDirName);
		PresetVirtualPath = TEXT("/") + FPaths::Combine(PluginName, PresetDirName);

		IFileManager::Get().MakeDirectory(*PresetDiskPath, true);
	}
}

void UAuroraPresetManager::Deinitialize()
{
}

void UAuroraPresetManager::SavePreset(UAuroraPresetBase* InPreset, const FString& NewPresetName)
{
	FString PresetPath = PresetVirtualPath + TEXT("/") + NewPresetName;
	UAuroraPresetBase* TargetAsset = Cast<UAuroraPresetBase>(StaticLoadObject(UAuroraPresetBase::StaticClass(), nullptr, *PresetPath));
	if (!TargetAsset)
	{
		TGuardValue<bool> PresetAddedDelegateGuard(FVolumetricAuroraEditorModule::bIsCreatingAssetWhileSaving, true);
		
		// FAssetToolsModule gets AssetTools interface, remains after module unloads
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TargetAsset = Cast<UAuroraPresetBase>(AssetTools.CreateAsset(NewPresetName, PresetVirtualPath, InPreset->GetClass(), nullptr));
	}
	if (TargetAsset)
	{
		TargetAsset->CopyFrom(InPreset);

		UPackage* TargetPackage = TargetAsset->GetPackage();

		if (TargetPackage)
		{
			TargetPackage->MarkPackageDirty();

			TArray<UPackage*> Packages;
			Packages.Add(TargetPackage);
			UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
			
		}
	}
}

FString UAuroraPresetManager::GetPresetVirtualPath() const
{
	return PresetVirtualPath;
}

FString UAuroraPresetManager::GetPluginName() const
{
	return PluginName;
}

FPresetNameValidationResult UAuroraPresetManager::ValidatePresetName(const FString& InName)
{
	FText ErrorMessage;
	if (!FName(InName).IsValidObjectName(ErrorMessage))
	{

		return FPresetNameValidationResult::Failure(ErrorMessage);
	}
	else if (InName.IsEmpty())
	{
		ErrorMessage = INVTEXT("Preset name must be specified");
		return FPresetNameValidationResult::Failure(ErrorMessage);
	}
	return FPresetNameValidationResult::Success();
}
