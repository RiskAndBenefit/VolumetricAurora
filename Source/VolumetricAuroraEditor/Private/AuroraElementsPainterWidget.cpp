// Copyright (c) 2026 R&B. All rights reserved.

#include "AuroraElementsPainterWidget.h"
#include "SAuroraPreviewViewport.h"

#include "VolumetricAurora.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/SavePackage.h"

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

void UAuroraElementsPainterWidget::BakeToTexture()
{
	// ========================================================================
	// Step 1: Validate prerequisites
	// ========================================================================

	if (!TargetAurora)
	{
		UE_LOG(LogTemp, Error, TEXT("BakeToTexture: TargetAurora is null"));
		return;
	}

	if (!CurrentRenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("BakeToTexture: CurrentRenderTarget is null"));
		return;
	}

	UTextureRenderTarget2D* RT = CurrentRenderTarget;
	
	// ========================================================================
	// Step 2: Read pixels from RenderTarget (GPU -> CPU)
	// ========================================================================

	TArray<FColor> SurfaceData;

	FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogTemp, Error, TEXT("BakeToTexture: Failed to get RenderTarget resource"));
		return;
	}

	// ReadPixels: Blocking operation, waits for GPU to finish rendering
	if (!RTResource->ReadPixels(SurfaceData))
	{
		UE_LOG(LogTemp, Error, TEXT("BakeToTexture: Failed to read pixels from RenderTarget"));
		return;
	}

	// ========================================================================
	// Step 3: Generate asset name with timestamp
	// ========================================================================

	FString AssetName = FString::Printf(
		TEXT("T_AuroraElements_%s_%s"),
		*TargetAurora->GetName(),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))
	);

	// Asset path: /VolumetricAurora/ refers to plugin mount point
	FString PackagePath = TEXT("/VolumetricAurora/Textures/FlowElementMap/");
	FString PackageName = PackagePath + AssetName;

	// ========================================================================
	// Step 4: Create Texture2D asset
	// ========================================================================

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	// RF_Public: Accessible from other packages
	// RF_Standalone: Won't be GC'd automatically
	// RF_MarkAsRootSet: Prevents garbage collection
	UTexture2D* NewTexture = NewObject<UTexture2D>(
		Package,
		*AssetName,
		RF_Public | RF_Standalone | RF_MarkAsRootSet
	);

	// ========================================================================
	// Step 5: Initialize texture source data
	// ========================================================================

	NewTexture->Source.Init(
		RT->SizeX,				// Width
		RT->SizeY,				// Height
		1,						// NumSlices (1 for 2D texture)
		1,						// NumMips (generate mips later)
		TSF_BGRA8				// Source format (matches FColor)
	);

	// ========================================================================
	// Step 6: Copy pixel data to Texture2D
	// ========================================================================

	// Lock texture for writing
	uint8* MipData = NewTexture->Source.LockMip(0);

	// Memcpy: Fast memory copy
	FMemory::Memcpy(
		MipData,
		SurfaceData.GetData(),
		SurfaceData.Num() * sizeof(FColor)
	);

	// Unlock texture
	NewTexture->Source.UnlockMip(0);

	// ========================================================================
	// Step 7: Configure texture settings
    // ========================================================================

	NewTexture->SRGB = false;									// Linear color space (data texture)
	NewTexture->CompressionSettings = TC_VectorDisplacementmap;	// RGB only, no alpha
	NewTexture->MipGenSettings = TMGS_NoMipmaps;				// No mipmaps needed
	NewTexture->AddressX = TA_Clamp;							// Clamp at edges
	NewTexture->AddressY = TA_Clamp;
	NewTexture->Filter = TF_Bilinear;							// Bilinear filtering
	NewTexture->AlphaCoverageThresholds = FVector4(0, 0, 0, 0);

	// Build texture (compile for GPU)
	NewTexture->UpdateResource();

	// ========================================================================
	// Step 8: Save asset to disk
	// ========================================================================

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	// Convert package name to file path
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension()
	);

	// Configure save parameters
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = false;
	SaveArgs.bWarnOfLongFilename = true;

	// Save package
	FSavePackageResultStruct SaveResult = UPackage::Save(
		Package,
		NewTexture,
		*PackageFileName,
		SaveArgs
	);

	bool bSaved = (SaveResult.Result == ESavePackageResult::Success);

	// ========================================================================
	// Step 9: Auto-assign to TargetAurora
	// ========================================================================

	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("BakeToTexture: Texture saved successfully: %s"), *PackageName);

		if (UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora->TargetAurora))
		{
			FlowPreset->Modify();		// Mark for undo system
			FlowPreset->AuroraElementsMap = NewTexture;
			UE_LOG(LogTemp, Log, TEXT("BakeToTexture: Assigned to AuroraElementsMap"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BakeToTexture: Failed to save texture"));
	}
}
