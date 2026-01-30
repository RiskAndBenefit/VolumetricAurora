// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AuroraPresetManager.h"
#include "Widgets/SCompoundWidget.h"

class USplineDFTextureBakerComponent;

class VOLUMETRICAURORAEDITOR_API SSaveDFTextureAsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSaveDFTextureAsWidget)
		{
		}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(USplineDFTextureBakerComponent*, BakerComponent)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void OnAssetSelected(const FAssetData& AssetData);

	void OnAssetDoubleClicked(const FAssetData& AssetData);

	void OnTextChanged(const FText& InText);

	FReply OnSaveClicked();

	FReply OnCancelClicked();

private:
	USplineDFTextureBakerComponent* BakerComponent;

	UAuroraPresetManager* PresetManager = nullptr;

	TWeakPtr<SWindow> ParentWindow;

	TSharedPtr<SEditableTextBox> NameInputBox;

	FPresetNameValidationResult ValidationResult;
};