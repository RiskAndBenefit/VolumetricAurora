// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuroraElementsPainterWidget.generated.h"

// Forward declarations
class UTextureRenderTarget2D;
class SAuroraPreviewViewport;

/**
 * @brief Base class for Aurora Elements Painter Widget
 *
 * This class provides C++ interface for WBP_AuroraElementsPainter.
 * Blueprint should inherit from this class to implement painting logic.
 *
 * Required Blueprint Functions:
 * - InitializePainter: Called when Editor opens the painter
 * - SetBrushSize: Called when user changes brush size (optional)
 * - SetBrushChannel: Called when user switches R/G channel (optional)
 */
UCLASS(Abstract, Blueprintable)
class VOLUMETRICAURORAEDITOR_API UAuroraElementsPainterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initialize painter with target render target
	 *
	 * This function is called by Editor when the paint window opens.
	 * Blueprint MUST implement this to set up DrawToRenderTarget logic.
	 *
	 * @param InRenderTarget The ElementsRenderTarget from VolumetricAurora
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aurora Painter")
	void InitializePainter(UTextureRenderTarget2D* InRenderTarget);

	/**
	 * @brief Called when user clicks "Apply" or "Bake"
	 *
	 * Optional: Implement cleanup or finalization logic in Blueprint.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aurora Painter")
	void OnPaintingComplete();

	/**
	 * @brief Refresh canvas display after Load() operation
	 *
	 * Called by C++ after Load() copies ShapeTexture to CurrentRenderTarget.
	 * Blueprint should implement this to update Canvas RT (the render target
	 * displayed in Canvas widget) with CurrentRenderTarget content.
	 *
	 * Typical implementation in Blueprint:
	 * 1. Draw Material To Render Target (Canvas RT, Material copying CurrentRenderTarget)
	 * 2. Or copy CurrentRenderTarget pixels directly to Canvas RT
	 *
	 * This ensures the visual canvas matches the loaded element map.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aurora Painter")
	void RefreshCanvas();

	/**
	 * @brief Set preview render target to display in preview image
	 *
	 * Called from C++ (VolumetricAuroraDetailsCustomization) after:
	 * - Preview aurora system is initialized
	 * - SceneCapture has rendered initial frame
	 *
	 * @param RenderTarget Captured preview texture from SceneCapture2D
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void SetPreviewRenderTarget(UTextureRenderTarget2D* RenderTarget);

	/**
	 * @brief Trigger preview aurora update after canvas drawing
	 *
	 * Blueprint should call this function after:
	 * - DrawMaterialToRenderTarget execution
	 * - Any modification to CurrentRenderTarget
	 *
	 * This updates the preview aurora's ShapeTexture and re-captures the scene.
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void UpdatePreview();

	/**
	 * @brief Load existing ShapeTexture into paint canvas
	 *
	 * Copies the current actor's ShapeTexture content to CurrentRenderTarget (paint canvas).
	 * This allows users to load and edit existing element maps.
	 *
	 * Workflow:
	 * 1. Validate TargetAurora and CurrentRenderTarget exist
	 * 2. Get PotentialFlowAuroraPreset from TargetAurora
	 * 3. If ShapeTexture exists, copy it to canvas using GPU-based material rendering
	 * 4. If ShapeTexture is null, clear canvas to black (default empty state)
	 *
	 * Use cases:
	 * - User clicks "Load from Element Map" button to sync canvas with current map
	 * - Auto-load when paint window opens (optional)
	 * - Reset canvas to original element map after mistakes
	 *
	 * @note This function only works with PotentialFlowAuroraPreset.
	 *       NoiseAuroraPreset and SplineAuroraPreset use ShapeTexture differently.
	 *
	 * @note If ShapeTexture is null, the canvas will be cleared to black instead of failing.
	 *       This provides a clean starting point for painting new element maps.
	 *
	 * @warning Requires M_Copy material to exist at /VolumetricAurora/Materials/M_Copy.
	 *          The material should have a TextureSampleParameter2D named "SourceTexture".
	 *
	 * @see Save() for reverse operation (canvas → texture)
	 * @see SaveAs() for creating new texture from canvas
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void Load();

	/**
	 * @brief Save current ElementsRenderTarget into the existing ShapeTexture asset.
	 *
	 * This function updates the already assigned Texture2D (ShapeTexture) by overwriting its pixel data
	 * with the current painter render target.
	 *
	 * Workflow:
	 * 1. Validate TargetAurora, CurrentRenderTarget, and flow preset
	 * 2. If ShapeTexture is null, fallback to SaveAs()
	 * 3. Read pixels from CurrentRenderTarget (GPU -> CPU)
	 * 4. Load the existing Texture2D asset and overwrite its Source mip0
	 * 5. Mark package dirty and save package to disk
	 *
	 * Notes:
	 * - This is an editor-only operation and does not create a new asset.
	 * - The existing asset must be a UTexture2D with editable source data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void Save();
	
	/**
	 * @brief Bake ElementsRenderTarget to new Texture2D asset
	 *
	 * Workflow:
	 * 1. Read pixels from ElementsRenderTarget (GPU -> CPU)
	 * 2. Create new Texture2D asset in Plugin Content folder
	 * 3. Copy pixel data to Texture2D
	 * 4. Save asset to disk
	 * 5. Auto-assign to TargetAurora's ShapeTexture
	 *
	 * Asset location: /VolumetricAurora/Textures/FlowElementMaps/
	 * Naming: T_AuroraElements_{ActorName}_{TimeStamp}.uasset
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void SaveAs();
	
	/**
	 * @brief Reference to the VolumetricAurora actor being edited
	 * Set by C++ when paint window opens
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Aurora Painter")
	TObjectPtr<class AVolumetricAurora> TargetAurora;
	
	/**
	 * @brief Get current render target being painted
	 *
	 * Blueprint can expose this variable for read access.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Aurora Painter")
	TObjectPtr<UTextureRenderTarget2D> CurrentRenderTarget;

	// ========================================================================
	// Orbit Camera Viewport Integration
	// ========================================================================

	/**
	 * @brief Cached reference to the orbit camera viewport Slate widget
	 *
	 * This widget provides Material Editor-style camera controls:
	 * - Left Mouse Drag: Orbit around aurora
	 * - Mouse Wheel: Zoom in/out
	 *
	 * The viewport is created by VolumetricAuroraDetailsCustomization and
	 * placed alongside this widget using SSplitter layout.
	 * This reference is available for Blueprint access if needed.
	 */
	TSharedPtr<SAuroraPreviewViewport> PreviewViewportWidget;
};