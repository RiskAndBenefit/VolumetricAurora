// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuroraElementsPainterWidget.generated.h"

// Forward declarations
class UTextureRenderTarget2D;

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
	 * This updates the preview aurora's AuroraElementsMap and re-captures the scene.
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void UpdatePreview();

	/**
	 * @brief Bake ElementsRenderTarget to permanent Texture2D asset
	 *
	 * Workflow:
	 * 1. Read pixels from ElementsRenderTarget (GPU -> CPU)
	 * 2. Create new Texture2D asset in Plugin Content folder
	 * 3. Copy pixel data to Texture2D
	 * 4. Save asset to disk
	 * 5. Auto-assign to TargetAurora's AuroraElementsMap
	 *
	 * Asset location: /VolumetricAurora/Textures/FlowElementMaps/
	 * Naming: T_AuroraElements_{ActorName}_{TimeStamp}.uasset
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora Painter")
	void BakeToTexture();
	
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

	/**
	 * @brief Image widget that displays preview aurora render
	 *
	 * BindWidget meta:
	 * - Automatically binds to widget named "PreviewImage" in WBP
	 * - Compile error if widget with matching name doesn't exist
	 * - Ensures type safety (must be UImage)
	 */
	UPROPERTY(meta = (BindWidget))
	class UImage* PreviewImage;
};