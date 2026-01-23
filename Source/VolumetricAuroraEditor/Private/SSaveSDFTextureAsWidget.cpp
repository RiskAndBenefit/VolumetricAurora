
// Fill out your copyright notice in the Description page of Project Settings.

#include "SSaveSDFTextureAsWidget.h"
#include "SplineSDFTextureBakerComponent.h"
#include "SlateOptMacros.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSaveSDFTextureAsWidget::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	BakerComponent = InArgs._BakerComponent;

	PresetManager = GEditor->GetEditorSubsystem<UAuroraPresetManager>();

	FString VirtualPath = TEXT("/") + PresetManager->GetPluginName() + TEXT("/Textures/SDFTextures");

	FAssetPickerConfig Config;
	Config.Filter.Clear();
	Config.Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());

	Config.Filter.bRecursivePaths = true;
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SSaveSDFTextureAsWidget::OnAssetSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SSaveSDFTextureAsWidget::OnAssetDoubleClicked);
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
								.OnTextChanged(this, &SSaveSDFTextureAsWidget::OnTextChanged)
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
										.OnClicked(this, &SSaveSDFTextureAsWidget::OnSaveClicked)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SButton)
										.Text(FText::FromString("Cancel"))
										.OnClicked(this, &SSaveSDFTextureAsWidget::OnCancelClicked)
								]
						]
				]

		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSaveSDFTextureAsWidget::OnAssetSelected(const FAssetData& AssetData)
{
	NameInputBox->SetText(FText::FromName(AssetData.AssetName));
}

void SSaveSDFTextureAsWidget::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	OnAssetSelected(AssetData);
	OnSaveClicked();
}

void SSaveSDFTextureAsWidget::OnTextChanged(const FText& InText)
{
	ValidationResult = PresetManager->ValidatePresetName(InText.ToString());
}

FReply SSaveSDFTextureAsWidget::OnSaveClicked()
{
	if (PresetManager)
	{
		FString NewTextureName = NameInputBox->GetText().ToString();
		ValidationResult = PresetManager->ValidatePresetName(NewTextureName);

		if (!ValidationResult.bIsValid)
		{
			return FReply::Handled();
		}

		BakerComponent->MakeSDFTexture(NewTextureName);

		if (ParentWindow.IsValid())
		{
			ParentWindow.Pin()->RequestDestroyWindow();
		}
	}
	return FReply::Handled();
}

FReply SSaveSDFTextureAsWidget::OnCancelClicked()
{
	if (ParentWindow.IsValid())
		ParentWindow.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}