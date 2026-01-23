// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AuroraPresetManager.generated.h"

class UAuroraPresetBase;

struct FPresetNameValidationResult
{
	bool bIsValid = true;
	FText ErrorMessage;

	static FPresetNameValidationResult Success() { return { true, FText::GetEmpty() }; }
	static FPresetNameValidationResult Failure(const FText& InError) { return { false, InError }; }
};
/**
 * Save, Load Aurora Preset
 */
UCLASS()
class VOLUMETRICAURORAEDITOR_API UAuroraPresetManager : public UEditorSubsystem
{
	GENERATED_BODY()
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
public:

	void SavePreset(UAuroraPresetBase* InPreset, const FString& NewPresetName);

	FString GetPresetVirtualPath() const;

	FString GetPluginName() const;

	FPresetNameValidationResult ValidatePresetName(const FString& InName);


private:

	FString PluginName;

	FString PresetDiskPath;

	FString PresetVirtualPath;
};
