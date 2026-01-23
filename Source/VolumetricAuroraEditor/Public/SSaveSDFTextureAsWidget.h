#pragma once

#include "CoreMinimal.h"
#include "AuroraPresetManager.h"
#include "Widgets/SCompoundWidget.h"

class USplineSDFTextureBakerComponent;

class VOLUMETRICAURORAEDITOR_API SSaveSDFTextureAsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSaveSDFTextureAsWidget)
		{
		}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(USplineSDFTextureBakerComponent*, BakerComponent)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void OnAssetSelected(const FAssetData& AssetData);

	void OnAssetDoubleClicked(const FAssetData& AssetData);

	void OnTextChanged(const FText& InText);

	FReply OnSaveClicked();

	FReply OnCancelClicked();

private:
	USplineSDFTextureBakerComponent* BakerComponent;

	UAuroraPresetManager* PresetManager = nullptr;

	TWeakPtr<SWindow> ParentWindow;

	TSharedPtr<SEditableTextBox> NameInputBox;

	FPresetNameValidationResult ValidationResult;
};