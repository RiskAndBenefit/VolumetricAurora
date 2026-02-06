// Copyright (c) 2026 R&B. All rights reserved.

#include "Widgets/AuroraElementsPainterWidget.h"
#include "Widgets/SaveFlowElementMapAsWidget.h"

#include "Actors/VolumetricAurora.h"
#include "Data/AuroraPresetAsset.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetRenderingLibrary.h"

void UAuroraElementsPainterWidget::SetPreviewRenderTarget(UTextureRenderTarget2D* RenderTarget)
{
	// ========================================================================
	// SetPreviewRenderTarget: Configure Preview Display
	// ========================================================================
	//
	// NOTE: The interactive preview viewport (SAuroraPreviewViewport) is now
	// created and managed by VolumetricAuroraDetailsCustomization, which places
	// it alongside this widget using SSplitter layout.
	
	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("SetPreviewRenderTarget: RenderTarget is null"));
		return;
	}

	// Debug: Verify RenderTarget properties
	FTextureResource* Resource = RenderTarget->GetResource();
	UE_LOG(LogTemp, Log, TEXT("SetPreviewRenderTarget: Success"));
	UE_LOG(LogTemp, Log, TEXT("  - RenderTarget: %p"), RenderTarget);
	UE_LOG(LogTemp, Log, TEXT("  - Resource: %p"), Resource);
	UE_LOG(LogTemp, Log, TEXT("  - Size: %dx%d"), RenderTarget->SizeX, RenderTarget->SizeY);
}

void UAuroraElementsPainterWidget::UpdatePreview()
{
	if (!TargetAurora)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdatePreview: TargetAurora is null"));
		return;
	}

	// Update preview aurora and Trigger scene capture
	TargetAurora->UpdatePreviewAurora();
}

void UAuroraElementsPainterWidget::Load()
{
	// Helper lambda to show error dialog with user-friendly message
	auto ShowErrorDialog = [](const FString& UserMessage, const FString& LogMessage)
	{
		UE_LOG(LogTemp, Error, TEXT("Load: %s"), *LogMessage);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(UserMessage));
	};

	// Helper lambda to show info dialog
	auto ShowInfoDialog = [](const FString& UserMessage, const FString& LogMessage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Load: %s"), *LogMessage);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(UserMessage));
	};

	// ========================================================================
	// Step 1: Validate paint canvas exists
	// ========================================================================

	// CurrentRenderTarget is the canvas where users paint aurora elements.
	// If it doesn't exist, we can't load anything into it.
	if (!CurrentRenderTarget)
	{
		ShowErrorDialog(
			TEXT("Failed to load: Canvas is not initialized.\nPlease reopen the Element Map editor."),
			TEXT("CurrentRenderTarget is null. Cannot load to canvas.")
		);
		return;
	}

	// ========================================================================
	// Step 2: Validate TargetAurora exists
	// ========================================================================

	if (!TargetAurora)
	{
		ShowErrorDialog(
			TEXT("Failed to load: Aurora actor not found.\nPlease ensure the actor is properly initialized."),
			TEXT("TargetAurora is null")
		);
		return;
	}

	// ========================================================================
	// Step 3: Get PotentialFlowAuroraPreset from TargetAurora
	// ========================================================================

	// Only PotentialFlowAuroraPreset uses ShapeTexture as element map.
	// NoiseAuroraPreset and SplineAuroraPreset use ShapeTexture differently.
	UPotentialFlowAuroraPreset* FlowPreset =
		Cast<UPotentialFlowAuroraPreset>(TargetAurora->TargetAurora);
	if (!FlowPreset)
	{
		ShowErrorDialog(
			TEXT("Failed to load: This preset type doesn't support Element Maps.\nOnly Potential Flow Aurora presets can be edited."),
			TEXT("TargetAurora is not PotentialFlowAuroraPreset. Element map editing is only supported for Flow presets.")
		);
		return;
	}

	// ========================================================================
	// Step 4: Check if ShapeTexture exists
	// ========================================================================

	// ShapeTexture contains the aurora element map (particle emission regions).
	// If it's null, we clear the canvas instead of copying.
	if (!FlowPreset->ShapeTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Load: ShapeTexture is null. Clearing canvas to black instead."));

		// Clear canvas to default black color (no emission)
		UKismetRenderingLibrary::ClearRenderTarget2D(
			this,
			CurrentRenderTarget,
			FLinearColor::Black
		);

		ShowInfoDialog(
			TEXT("No existing Element Map found.\nCanvas cleared to black (empty state)."),
			TEXT("ShapeTexture is null. Canvas cleared to black.")
		);
		return;
	}

	// ========================================================================
	// Step 5: Copy ShapeTexture to CurrentRenderTarget using material
	// ========================================================================

	// We can't directly copy texture data in UE5 without GPU involvement.
	// Instead, we use a simple material that samples the source texture
	// and draws it to the render target.

	// Create dynamic material instance configured to copy ShapeTexture
	UMaterialInstanceDynamic* CopyMaterial =
		TargetAurora->CreateSimpleMaterialForTextureCopy(FlowPreset->ShapeTexture);

	if (!CopyMaterial)
	{
		ShowErrorDialog(
			TEXT("Failed to load: Cannot create material for texture copy.\n\nPlease ensure M_Copy material exists at:\n/VolumetricAurora/Materials/M_Copy\n\nThe material should have a TextureSampleParameter2D named 'SourceTexture' connected to Emissive Color."),
			TEXT("Failed to create copy material. Check M_Copy material exists in plugin.")
		);
		return;
	}

	// Draw the copy material to CurrentRenderTarget
	// This effectively copies ShapeTexture pixel data to the canvas
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(
		this,
		CurrentRenderTarget,
		CopyMaterial
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Load: Successfully loaded ShapeTexture '%s' to canvas"),
		*FlowPreset->ShapeTexture->GetName()
	);

	// ========================================================================
	// Step 6: Refresh Canvas RT (Blueprint implementation)
	// ========================================================================

	// CRITICAL: WBP uses separate Canvas RT for display!
	// CurrentRenderTarget is updated above, but Canvas RT (shown in Canvas widget)
	// needs to be refreshed with CurrentRenderTarget content.
	// Blueprint implements this to copy CurrentRenderTarget → Canvas RT.
	RefreshCanvas();

	// Note: No success dialog shown - visual feedback from canvas update is sufficient
}

void UAuroraElementsPainterWidget::Save()
{
	// Helper lambda to show error dialog with user-friendly message
	auto ShowErrorDialog = [](const FString& UserMessage, const FString& LogMessage)
	{
		UE_LOG(LogTemp, Error, TEXT("Save: %s"), *LogMessage);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(UserMessage));
	};

	// Validate prerequisites
	if (!TargetAurora)
	{
		ShowErrorDialog(
			TEXT("Failed to save: Aurora actor not found.\nPlease ensure the actor is properly initialized."),
			TEXT("TargetAurora is null")
		);
		return;
	}

	if (!CurrentRenderTarget)
	{
		ShowErrorDialog(
			TEXT("Failed to save: Canvas is not initialized.\nPlease reopen the Element Map editor."),
			TEXT("CurrentRenderTarget is null")
		);
		return;
	}

	UPotentialFlowAuroraPreset* FlowPreset =
		Cast<UPotentialFlowAuroraPreset>(TargetAurora->TargetAurora);
	if (!FlowPreset)
	{
		ShowErrorDialog(
			TEXT("Failed to save: This preset type doesn't support Element Maps.\nOnly Potential Flow Aurora presets can be edited."),
			TEXT("Aurora preset is null or is not flow type")
		);
		return;
	}

	// If no existing map, Save() cannot overwrite -> create a new one.
	if (!FlowPreset->ShapeTexture)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Save: ShapeTexture is null. Falling back to SaveAs().")
		);
		
		SaveAs();
		return;
	}

	UTextureRenderTarget2D* RT = CurrentRenderTarget;

	// Read pixels from RenderTarget (GPU -> CPU)
	TArray<FColor> SurfaceData;

	FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		ShowErrorDialog(
			TEXT("Failed to save: Cannot access canvas render data.\nThe canvas may not be fully initialized. Try reopening the editor."),
			TEXT("Failed to get RenderTarget resource")
		);
		return;
	}

	// ReadPixels is a blocking call (flushes GPU work for readback).
	if (!RTResource->ReadPixels(SurfaceData))
	{
		ShowErrorDialog(
			TEXT("Failed to save: Cannot read pixel data from canvas.\nThis may be a GPU access issue. Try saving again."),
			TEXT("Failed to read pixels from RenderTarget")
		);
		return;
	}

	// Load existing Texture2D asset correctly
	// ShapeTexture may be UTexture or UTexture2D.
	UTexture2D* ExistingTexture = Cast<UTexture2D>(FlowPreset->ShapeTexture);
	if (!ExistingTexture)
	{
		ShowErrorDialog(
			TEXT("Failed to save: The Element Map is not a valid Texture2D asset.\nIt cannot be overwritten. Use 'Save As' to create a new texture."),
			TEXT("ShapeTexture is not a Texture2D (cannot overwrite source pixels)")
		);
		return;
	}

	// Ensure size matches (overwriting mismatched source is unsafe)
	if (ExistingTexture->GetSizeX() != RT->SizeX || ExistingTexture->GetSizeY() != RT->SizeY)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Save: Size mismatch. Texture=%dx%d, RT=%dx%d. Recreating asset via SaveAs()."),
			ExistingTexture->GetSizeX(), ExistingTexture->GetSizeY(), RT->SizeX, RT->SizeY);
		
		SaveAs();
		return;
	}

	// Get owning package (this is the correct package to mark dirty & save).
	UPackage* Package = ExistingTexture->GetOutermost();
	if (!Package)
	{
		ShowErrorDialog(
			TEXT("Failed to save: Cannot access the texture's package.\nThe texture asset may be corrupted."),
			TEXT("Failed to get outermost package from existing texture")
		);
		return;
	}

	// Make sure package is fully loaded before editing
	Package->FullyLoad();

	// Overwrite pixel data (CPU memory -> Texture Source)
	// Update existing asset content (supports undo/redo).
	ExistingTexture->Modify();
	Package->Modify();

	// Copy raw BGRA8 pixels into mip0.
	uint8* MipData = ExistingTexture->Source.LockMip(0);
	if (!MipData)
	{
		ShowErrorDialog(
			TEXT("Failed to save: Cannot lock texture for editing.\nThe texture may be in use or corrupted."),
			TEXT("Failed to lock mip0 for writing")
		);
		return;
	}

	const int32 ExpectedBytes = RT->SizeX * RT->SizeY * sizeof(FColor);
	const int32 SourceBytes = SurfaceData.Num() * sizeof(FColor);

	if (SourceBytes < ExpectedBytes)
	{
		ExistingTexture->Source.UnlockMip(0);
		ShowErrorDialog(
			TEXT("Failed to save: Canvas data size mismatch.\nExpected size doesn't match actual canvas size."),
			TEXT("SurfaceData size is smaller than expected")
		);
		return;
	}

	FMemory::Memcpy(MipData, SurfaceData.GetData(), ExpectedBytes);
	ExistingTexture->Source.UnlockMip(0);

	// Push updated source to GPU.
	ExistingTexture->UpdateResource();

	// Save package to disk
	// Mark dirty so the editor knows the asset changed.
	Package->MarkPackageDirty();

	const FString PackageName = Package->GetName(); // Long package name: /Game/...
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension()
	);

	// Check whether the package file is read-only.
	// If so, abort Save() and fall back to SaveAs().
	if (IFileManager::Get().FileExists(*PackageFileName) &&
		IFileManager::Get().IsReadOnly(*PackageFileName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Save: Package file is read-only (cannot overwrite): %s"),
			*PackageFileName
		);

		// show warning log
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(
				TEXT("This texture file is read-only.\n")
				TEXT("The asset will be saved as a new file.")
			)
		);

		SaveAs();
		return;
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = true;

	FSavePackageResultStruct SaveResult = UPackage::Save(
		Package,
		ExistingTexture,
		*PackageFileName,
		SaveArgs
	);

	if (SaveResult.Result == ESavePackageResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("Save: Overwrote existing texture successfully: %s"), *PackageName);
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT("Element Map saved successfully!"))
		);
	}
	else
	{
		ShowErrorDialog(
			FString::Printf(
				TEXT("Failed to save: Cannot write texture file to disk.\nCheck file permissions and disk space.\n\nPackage: %s"),
				*PackageName
			),
			FString::Printf(TEXT("Failed to save package: %s"), *PackageName)
		);
	}
}


void UAuroraElementsPainterWidget::SaveAs()
{
	if (!TargetAurora)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveAs: TargetAurora is null"));
		return;
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString("Save Element Map as"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Window->SetContent(SNew(SSaveFlowElementMapAsWidget)
		.ParentWindow(Window)
		.TargetAurora(TargetAurora)
		.CurrentRenderTarget(CurrentRenderTarget)
	);

	FSlateApplication::Get().AddWindow(Window);
}
