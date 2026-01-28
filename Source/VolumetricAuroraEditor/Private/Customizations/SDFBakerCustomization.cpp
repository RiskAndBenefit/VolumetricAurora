// Copyright (c) 2026 R&B. All rights reserved.

#include "Customizations/SDFBakerCustomization.h"
#include "Components/SplineSDFTextureBakerComponent.h"
#include "Widgets/SaveSDFTextureAsWidget.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "Data/AuroraPresetAsset.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SNumericEntryBox.h"

TSharedRef<IDetailCustomization> FSDFBakerCustomization::MakeInstance()
{
	// Create shared pointer to customization instance
	// Unreal used TSharedRef for automatic memory management
	return MakeShareable(new FSDFBakerCustomization);
}

void FSDFBakerCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{

	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	if (CustomizedObjects.Num() > 0)
	{
		SelectedComponent = Cast<USplineSDFTextureBakerComponent>(CustomizedObjects[0].Get());
	}
	else
	{
		return;
	}

	IDetailCategoryBuilder& BakerCategory = DetailBuilder.EditCategory("SDFBaker");

	BakerCategory.AddCustomRow(FText::FromString("BakeSDFTexture"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString("Bake SDF Texture"))
					.OnClicked(this, &FSDFBakerCustomization::OnSaveAsButtonClicked)
					.VAlign(VAlign_Center)
				]
		];
	BakerCategory.AddCustomRow(FText::FromString("Save"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Center)
				[

					SNew(SButton)
						.Text(FText::FromString("Add Spline Component"))
						.OnClicked(this, &FSDFBakerCustomization::OnAddSplineComponentButtonClicked)
						.VAlign(VAlign_Center)
				]
			+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
						.Text(FText::FromString("Focus on the visualizer"))
						.OnClicked_Lambda([this]()
							{
								SelectedComponent->FocusOnVisualizer();
								return FReply::Handled();
							})
						.VAlign(VAlign_Center)
				]
		];
}

FReply FSDFBakerCustomization::OnAddSplineComponentButtonClicked()
{
	SelectedComponent->AddSplineComponent();
	return FReply::Handled();
}

FReply FSDFBakerCustomization::OnSaveAsButtonClicked()
{

	if (!SelectedComponent)
	{
		return FReply::Handled();
	}
	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Title(FText::FromString("Save Texture As"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	NewWindow->SetContent(SNew(SSaveSDFTextureAsWidget)
		.ParentWindow(NewWindow)
		.BakerComponent(SelectedComponent));
	

	FSlateApplication::Get().AddWindow(NewWindow);
	return FReply::Handled();
}
