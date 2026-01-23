// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AuroraPresetAsset.h"
#include "AuroraPresetManager.h"

struct FAuroraTypeInfo
{
	FString Name;
	FString Description;
	FSlateBrush AuroraBrush;

	TSubclassOf<UAuroraPresetBase> PresetClass;

	// 텍스처 로드 안되는 경우 예방
	FAuroraTypeInfo() {
		AuroraBrush = *FAppStyle::GetBrush("DefaultAssetIcon");
	}
};
/**
 * 
 */

class AVolumetricAurora;


class VOLUMETRICAURORAEDITOR_API SAuroraTypeSelectorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAuroraTypeSelectorWidget)
	{}
		// SWindow* _ParentWindow FArguments에 추가, FArguments& ParentWindow(SWindow* InValue) 추가
		SLATE_ARGUMENT(TSharedPtr<SWindow>, InParentWindow)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> CreateAuroraTypeButton(int Index, const FAuroraTypeInfo& InAuroraType);

	void OnTextChanged(const FText& InText);

	FReply OnAuroraConfirmClicked();

	FReply OnAuroraCancelClicked();

private:

	TSharedPtr<SWindow> ParentWindow;

	TArray<FAuroraTypeInfo> AuroraTypes;

	TSharedPtr<SEditableTextBox> PresetNameBox;

	UAuroraPresetManager* PresetManager = nullptr;

	FString PluginPath;

	int SelectedIndex = -1;

	FPresetNameValidationResult ValidationResult;

	void InitAuroraTypes();


};
