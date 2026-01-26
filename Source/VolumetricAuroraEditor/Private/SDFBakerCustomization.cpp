// Copyright (c) 2026 R&B. All rights reserved.

#include "SDFBakerCustomization.h"
#include "SplineSDFTextureBakerComponent.h"
#include "SSaveSDFTextureAsWidget.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "AuroraPresetAsset.h"
#include "DetailWidgetRow.h"
#include "SAuroraExtentWidget.h"
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

	BakerCategory.AddCustomRow(FText::FromString("Save"))
		.NameContent()
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
				.MinDesiredWidth(100.f)
				[
					SNew(SButton)
						.Text(FText::FromString("Add Spline Component"))
						.OnClicked(this, &FSDFBakerCustomization::OnAddSplineComponentButtonClicked)
						.HAlign(HAlign_Center)
				]
		]
	.ValueContent()
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
				.MinDesiredWidth(100.f)
				[
					SNew(SButton)
						.Text(FText::FromString("Bake SDF Texture"))
						.OnClicked(this, &FSDFBakerCustomization::OnSaveAsButtonClicked)
						.HAlign(HAlign_Center)
				]
		];
	/*TSharedPtr<IPropertyHandle> ExtentProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAuroraPresetBase, AuroraAreaExtent));
	
	if (ExtentProp.IsValid())
	{
		IDetailPropertyRow* PropRow = DetailBuilder.EditDefaultProperty(ExtentProp);

		if (PropRow)
		{
			PropRow->CustomWidget()
				.NameContent()
				[
					ExtentProp->CreatePropertyNameWidget()
				]
				.ValueContent()
				[
					SNew(SAuroraExtentWidget)
						.Value_Lambda([ExtentProp]()
							{
								float Value;
								ExtentProp->GetValue(Value);
								return Value;
							})
						.OnValueChanged_Lambda([ExtentProp](float Value)
							{
								ExtentProp->SetValue(Value,
									EPropertyValueSetFlags::InteractiveChange);
							})
						.Sensitivity(0.01f)
				];
		}
	}*/
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
