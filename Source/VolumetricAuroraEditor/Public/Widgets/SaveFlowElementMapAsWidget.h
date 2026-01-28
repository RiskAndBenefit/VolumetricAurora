// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AuroraPresetManager.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetNameValidationResult
{
	bool bIsValid = true;
	FText ErrorMessage;

	static FAssetNameValidationResult Success() { return { true, FText::GetEmpty() }; }
	static FAssetNameValidationResult Failure(const FText& InError) { return { false, InError }; }

};

class VOLUMETRICAURORAEDITOR_API SSaveFlowElementMapAsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSaveFlowElementMapAsWidget)
	{}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(TObjectPtr<class AVolumetricAurora>, TargetAurora)
		SLATE_ARGUMENT(TObjectPtr<UTextureRenderTarget2D>, CurrentRenderTarget)
	// 필요하면 flow element 입력
	SLATE_END_ARGS()

	/**
	 * @brief Constructs this widget with InArgs
	 */
	void Construct(const FArguments& InArgs);

	void OnAssetSelected(const FAssetData& AssetData) const;

	void OnAssetDoubleClicked(const FAssetData& AssetData);

	FAssetNameValidationResult ValidateFlowMapName(const FString& InName);
	
	void OnTextChanged(const FText& InText);

	FReply OnSaveClicked();

	FReply OnCancelClicked();

private:
	TWeakPtr<SWindow> ParentWindow;

	TObjectPtr<class AVolumetricAurora> TargetAurora;
	TObjectPtr<UTextureRenderTarget2D> CurrentRenderTarget;
	
	TSharedPtr<SEditableTextBox> NameInputBox;
	
	FAssetNameValidationResult ValidationResult;
};