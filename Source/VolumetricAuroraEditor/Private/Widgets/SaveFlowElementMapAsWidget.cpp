// Copyright (c) 2026 R&B. All rights reserved.

#include "Widgets/SaveFlowElementMapAsWidget.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Actors/VolumetricAurora.h"
#include "Data/AuroraPresetAsset.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSaveFlowElementMapAsWidget::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	TargetAurora = InArgs._TargetAurora;
	CurrentRenderTarget = InArgs._CurrentRenderTarget;

	FString FlowElementMapPath = "/VolumetricAurora/Textures/FlowElementMap";
	
	FAssetPickerConfig Config;
	Config.Filter.Clear();
	Config.Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	Config.Filter.bRecursivePaths = true;
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(
		this,
		&SSaveFlowElementMapAsWidget::OnAssetSelected
	);
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.bAllowRename = true;
	Config.OnShouldFilterAsset =
		FOnShouldFilterAsset::CreateLambda(
			[FlowElementMapPath](const FAssetData& AssetData)
			{
				bool bInPath = AssetData.PackagePath.ToString().StartsWith(FlowElementMapPath);
				return !bInPath;
			}
		);

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

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
								return ValidationResult.bIsValid
									? EVisibility::Hidden
									: EVisibility::Visible;
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
								.OnTextChanged(this, &SSaveFlowElementMapAsWidget::OnTextChanged)
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
										.OnClicked(this, &SSaveFlowElementMapAsWidget::OnSaveClicked)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SButton)
										.Text(FText::FromString("Cancel"))
										.OnClicked(this, &SSaveFlowElementMapAsWidget::OnCancelClicked)
								]
						]
				]
		];

	// Set default asset name with timestamp
	FString AssetName = FString::Printf(
		TEXT("T_AuroraElements_%s_%s"),
		*TargetAurora->GetName(),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))
	);

	NameInputBox->SetText(FText::FromString(AssetName));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSaveFlowElementMapAsWidget::OnAssetSelected(const FAssetData& AssetData) const
{
	NameInputBox->SetText(FText::FromName(AssetData.AssetName));
}

void SSaveFlowElementMapAsWidget::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	OnAssetSelected(AssetData);
	OnSaveClicked();
}

FAssetNameValidationResult SSaveFlowElementMapAsWidget::ValidateFlowMapName(const FString& InName)
{
	// 1) Normalize input
	const FString Name = InName.TrimStartAndEnd();

	// 2) Empty check first (better error message)
	if (Name.IsEmpty())
	{
		return FAssetNameValidationResult::Failure(INVTEXT("Flow map name must be specified."));
	}

	// 3) Object name validation (Unreal naming rules)
	FText NameError;
	if (!FName(Name).IsValidObjectName(NameError))
	{
		return FAssetNameValidationResult::Failure(NameError);
	}

	// 4) Validate full object path (package + object)
	const FString PackagePath = TEXT("/VolumetricAurora/Textures/FlowElementMap/");
	const FString PackageName = PackagePath + Name;
	const FString ObjectPath  = PackageName + TEXT(".") + Name;

	if (!FPackageName::IsValidObjectPath(ObjectPath))
	{
		return FAssetNameValidationResult::Failure(
			FText::FromString(FString::Printf(TEXT("Invalid asset path: %s"), *ObjectPath))
		);
	}

	// 5) Optional: detect overwrite target (allowed, but useful feedback)
	// If you want to keep the red error area strictly for errors, don't set Failure here.
	// You could store a separate bool like bWillOverwrite for UI.
	// const bool bExists = (LoadObject<UTexture2D>(nullptr, *ObjectPath) != nullptr);

	return FAssetNameValidationResult::Success();
}

void SSaveFlowElementMapAsWidget::OnTextChanged(const FText& InText)
{
	ValidationResult = ValidateFlowMapName(InText.ToString());
}

FReply SSaveFlowElementMapAsWidget::OnSaveClicked()
{
	// Always clear previous UI error on new attempt.
	ValidationResult = FAssetNameValidationResult::Success();

	// ========================================================================
	// Step 1: Validate prerequisites
	// ========================================================================

	if (!TargetAurora)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: TargetAurora is null"));
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: TargetAurora is null."))
		);
		return FReply::Handled();
	}

	if (!CurrentRenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: CurrentRenderTarget is null"));
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: CurrentRenderTarget is null."))
		);
		return FReply::Handled();
	}

	if (!NameInputBox.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: NameInputBox is invalid"));
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: Name input box is not valid."))
		);
		return FReply::Handled();
	}

	UTextureRenderTarget2D* RT = CurrentRenderTarget;

	// ========================================================================
	// Step 2: Read pixels from RenderTarget (GPU -> CPU)
	// ========================================================================

	TArray<FColor> SurfaceData;

	FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: Failed to get RenderTarget resource"));
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: Failed to get RenderTarget resource."))
		);
		return FReply::Handled();
	}

	// ReadPixels blocks until GPU finishes the render work.
	if (!RTResource->ReadPixels(SurfaceData))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: Failed to read pixels from RenderTarget"));
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: Failed to read pixels from RenderTarget."))
		);
		return FReply::Handled();
	}

	// ========================================================================
	// Step 3: Build / validate target asset path
	// ========================================================================

	const FString AssetName = NameInputBox->GetText().ToString().TrimStartAndEnd();
	if (AssetName.IsEmpty())
	{
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Please enter a valid name."))
		);
		return FReply::Handled();
	}

	const FString PackagePath = TEXT("/VolumetricAurora/Textures/FlowElementMap/");
	const FString PackageName = PackagePath + AssetName;                 // /VolumetricAurora/.../MyTex
	const FString ObjectPath  = PackageName + TEXT(".") + AssetName;     // /VolumetricAurora/.../MyTex.MyTex

	// Validate with ObjectPath (covers name + package rules).
	if (!FPackageName::IsValidObjectPath(ObjectPath))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: Invalid asset path: %s"), *ObjectPath);
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(FString::Printf(TEXT("Invalid asset name/path:\n%s"), *ObjectPath))
		);
		return FReply::Handled();
	}

	// ========================================================================
	// Step 4: Overwrite if exists, otherwise create new
	// ========================================================================

	// Principle (simple):
	// - If a texture asset already exists at ObjectPath, overwrite its source pixels.
	// - Otherwise, create a new package + texture and initialize its source.

	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);

	bool bIsNewAsset = false;
	UPackage* Package = nullptr;

	if (Texture)
	{
		// Existing asset: edit in-place.
		Package = Texture->GetOutermost();
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("SaveAs: Existing texture has no package: %s"), *ObjectPath);
			ValidationResult = FAssetNameValidationResult::Failure(
				FText::FromString(TEXT("Save failed: Existing texture has no package."))
			);
			return FReply::Handled();
		}

		Package->FullyLoad();
		Texture->Modify();
		Package->Modify();
	}
	else
	{
		// New asset: create package and texture object.
		Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("SaveAs: CreatePackage failed: %s"), *PackageName);
			ValidationResult = FAssetNameValidationResult::Failure(
				FText::FromString(FString::Printf(TEXT("Save failed: CreatePackage failed:\n%s"), *PackageName))
			);
			return FReply::Handled();
		}

		Package->FullyLoad();

		Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
		if (!Texture)
		{
			UE_LOG(LogTemp, Error, TEXT("SaveAs: Failed to create texture object: %s"), *ObjectPath);
			ValidationResult = FAssetNameValidationResult::Failure(
				FText::FromString(FString::Printf(TEXT("Save failed: Failed to create texture object:\n%s"), *ObjectPath))
			);
			return FReply::Handled();
		}

		// Initialize source only when creating.
		Texture->Source.Init(RT->SizeX, RT->SizeY, 1, 1, TSF_BGRA8);

		// Set properties once for new assets.
		Texture->SRGB = false;
		Texture->CompressionSettings = TC_VectorDisplacementmap;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->Filter = TF_Bilinear;
		Texture->AlphaCoverageThresholds = FVector4(0, 0, 0, 0);

		bIsNewAsset = true;
	}

	// If size changed, re-init source to match the render target.
	if (Texture->GetSizeX() != RT->SizeX || Texture->GetSizeY() != RT->SizeY)
	{
		Texture->Source.Init(RT->SizeX, RT->SizeY, 1, 1, TSF_BGRA8);
	}

	// ========================================================================
	// Step 5: Copy pixels into texture source and update GPU resource
	// ========================================================================

	uint8* MipData = Texture->Source.LockMip(0);
	if (!MipData)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: Failed to lock mip0 for writing: %s"), *ObjectPath);
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: Failed to lock texture mip for writing."))
		);
		return FReply::Handled();
	}

	const int32 ExpectedBytes = RT->SizeX * RT->SizeY * sizeof(FColor);
	const int32 SourceBytes = SurfaceData.Num() * sizeof(FColor);
	if (SourceBytes < ExpectedBytes)
	{
		Texture->Source.UnlockMip(0);
		UE_LOG(LogTemp, Error, TEXT("SaveAs: SurfaceData is smaller than expected: %s"), *ObjectPath);
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("Save failed: Pixel buffer size mismatch."))
		);
		return FReply::Handled();
	}

	FMemory::Memcpy(MipData, SurfaceData.GetData(), ExpectedBytes);
	Texture->Source.UnlockMip(0);

	Texture->UpdateResource();

	// ========================================================================
	// Step 6: Save package to disk
	// ========================================================================

	Package->MarkPackageDirty();

	if (bIsNewAsset)
	{
		// Register only when a brand-new asset is created.
		FAssetRegistryModule::AssetCreated(Texture);
	}

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension()
	);

	if (IPlatformFile::GetPlatformPhysical().IsReadOnly(*PackageFileName))
	{
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(TEXT("The asset file is read-only and cannot be overwritten."))
		);
		return FReply::Handled();
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = true;

	const FSavePackageResultStruct SaveResult = UPackage::Save(Package, Texture, *PackageFileName, SaveArgs);
	const bool bSaved = (SaveResult.Result == ESavePackageResult::Success);

	// ========================================================================
	// Step 7: Assign to preset on success
	// ========================================================================

	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: Failed to save package: %s"), *PackageName);
		ValidationResult = FAssetNameValidationResult::Failure(
			FText::FromString(FString::Printf(TEXT("Save failed: Failed to save package:\n%s"), *PackageName))
		);
		return FReply::Handled();
	}

	UE_LOG(LogTemp, Log, TEXT("SaveAs: Saved texture: %s"), *ObjectPath);

	if (UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora->TargetAurora))
	{
		FlowPreset->Modify();
		FlowPreset->AuroraElementsMap = Texture;
	}

	// Clear UI error on success (hides the red text block).
	ValidationResult = FAssetNameValidationResult::Success();

	// Optional: close dialog window after saving.
	// if (TSharedPtr<SWindow> W = ParentWindow.Pin()) { W->RequestDestroyWindow(); }

	return FReply::Handled();
}

FReply SSaveFlowElementMapAsWidget::OnCancelClicked()
{
	if (ParentWindow.IsValid())
		ParentWindow.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}
