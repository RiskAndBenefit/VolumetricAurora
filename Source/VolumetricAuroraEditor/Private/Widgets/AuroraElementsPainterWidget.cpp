// Copyright (c) 2026 R&B. All rights reserved.

#include "Widgets/AuroraElementsPainterWidget.h"
#include "Widgets/SaveFlowElementMapAsWidget.h"

#include "Actors/VolumetricAurora.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"

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

void UAuroraElementsPainterWidget::Save()
{
	// Validate prerequisites
	if (!TargetAurora)
	{
		UE_LOG(LogTemp, Error, TEXT("Save: TargetAurora is null"));
		return;
	}

	if (!CurrentRenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("Save: CurrentRenderTarget is null"))
		return;
	}

	UPotentialFlowAuroraPreset* FlowPreset =
		Cast<UPotentialFlowAuroraPreset>(TargetAurora->TargetAurora);
	if (!FlowPreset)
	{
		UE_LOG(LogTemp, Error, TEXT("Save: Aurora preset is null or is not flow type"));
		return;
	}

	// If no existing map, Save() cannot overwrite -> create a new one.
	if (!FlowPreset->AuroraElementsMap)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Save: AuroraElementsMap is null. Falling back to SaveAs().")
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
		UE_LOG(LogTemp, Error, TEXT("Save: Failed to get RenderTarget resource"));
		return;
	}

	// ReadPixels is a blocking call (flushes GPU work for readback).
	if (!RTResource->ReadPixels(SurfaceData))
	{
		UE_LOG(LogTemp, Error, TEXT("Save: Failed to read pixels from RenderTarget"));
		return;
	}

	// Load existing Texture2D asset correctly
	// AuroraElementsMap may be UTexture or UTexture2D.
	UTexture2D* ExistingTexture = Cast<UTexture2D>(FlowPreset->AuroraElementsMap);
	if (!ExistingTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("Save: AuroraElementsMap is not a Texture2D (cannot overwrite source pixels)."));
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
		UE_LOG(LogTemp, Error, TEXT("Save: Failed to get outermost package from existing texture."));
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
		UE_LOG(LogTemp, Error, TEXT("Save: Failed to lock mip0 for writing."));
		return;
	}

	const int32 ExpectedBytes = RT->SizeX * RT->SizeY * sizeof(FColor);
	const int32 SourceBytes = SurfaceData.Num() * sizeof(FColor);

	if (SourceBytes < ExpectedBytes)
	{
		ExistingTexture->Source.UnlockMip(0);
		UE_LOG(LogTemp, Error, TEXT("Save: SurfaceData size is smaller than expected."));
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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Save: Failed to save package: %s"), *PackageName);
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
