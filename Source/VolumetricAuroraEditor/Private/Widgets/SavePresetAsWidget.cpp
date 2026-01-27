// Copyright (c) 2026 R&B. All rights reserved.

#include "Widgets/SavePresetAsWidget.h"
#include "Data/AuroraPresetAsset.h"
#include "SlateOptMacros.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSavePresetAsWidget::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	SourcePreset = InArgs._SourcePreset;

	PresetManager = GEditor->GetEditorSubsystem<UAuroraPresetManager>();

	FString VirtualPath = PresetManager->GetPresetVirtualPath();

	FAssetPickerConfig Config;
	Config.Filter.Clear();
	Config.Filter.ClassPaths.Add(UAuroraPresetBase::StaticClass()->GetClassPathName());
	
	Config.Filter.bRecursiveClasses = true;
	Config.Filter.bRecursivePaths = true;
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SSavePresetAsWidget::OnAssetSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SSavePresetAsWidget::OnAssetDoubleClicked);
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.bAllowRename = true;
	Config.OnShouldFilterAsset = FOnShouldFilterAsset::CreateLambda([VirtualPath](const FAssetData& AssetData)
		{
			bool bInPath = AssetData.PackagePath.ToString().StartsWith(VirtualPath);
			return !bInPath; 
		});

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	
	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f) 
				[
					SNew(SBox)
						.MinDesiredWidth(400.f)
						.MinDesiredHeight(300.f)
						.MaxDesiredHeight(500.0f)
						[
							ContentBrowserModule.Get().CreateAssetPicker(Config)
						]
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5.f)
				[
					SNew(STextBlock)
						.Text_Lambda([this]() { return ValidationResult.ErrorMessage; })
						.ColorAndOpacity(FLinearColor::Red)
						.Visibility_Lambda([this]()
							{
								return ValidationResult.bIsValid ? EVisibility::Hidden : EVisibility::Visible;
							})
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5.f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString("Name: "))
						]
						+ SHorizontalBox::Slot()
						.MaxWidth(205)
						[
							SAssignNew(NameInputBox, SEditableTextBox)
								.OnTextChanged(this, &SSavePresetAsWidget::OnTextChanged)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(5.f)
								[
									SNew(SButton)
										.Text(FText::FromString("Save"))
										.OnClicked(this, &SSavePresetAsWidget::OnSaveClicked)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SButton)
										.Text(FText::FromString("Cancel"))
										.OnClicked(this, &SSavePresetAsWidget::OnCancelClicked)
								]
						]
				]
			
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSavePresetAsWidget::OnAssetSelected(const FAssetData& AssetData)
{
	NameInputBox->SetText(FText::FromName(AssetData.AssetName));
}

void SSavePresetAsWidget::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	OnAssetSelected(AssetData);
	OnSaveClicked();
}

void SSavePresetAsWidget::OnTextChanged(const FText& InText)
{
	ValidationResult = PresetManager->ValidatePresetName(InText.ToString());
}

FReply SSavePresetAsWidget::OnSaveClicked()
{
	if (PresetManager)
	{
		FString NewPresetName = NameInputBox->GetText().ToString();
		ValidationResult = PresetManager->ValidatePresetName(NewPresetName);

		if (!ValidationResult.bIsValid)
		{
			return FReply::Handled();
		}

		PresetManager->SavePreset(SourcePreset, NewPresetName);

		if (ParentWindow.IsValid())
		{
			ParentWindow.Pin()->RequestDestroyWindow();
		}
	}
	return FReply::Handled();
}

FReply SSavePresetAsWidget::OnCancelClicked()
{
	if (ParentWindow.IsValid())
		ParentWindow.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}