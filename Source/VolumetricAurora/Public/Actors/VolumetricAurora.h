// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Types/AuroraTypes.h"
#include "Data/AuroraPresetAsset.h"
#include "ComputeShaders/AuroraFlowSimulateCS.h"

#include "Types/AuroraFlowElement.h"

#include "VolumetricAurora.generated.h"

// Forward declaration for Editor-only Slate widget
class SAuroraPreviewViewport;

namespace AuroraFlowConstants
{
	/* Basic */
	constexpr float BaseFlowScale = 0.5f;

	/* Control Point - Vortex, Sink, Spiral */
	constexpr float FadeStrengthScale = 10.f;

	/* Control Point - Curl */
	constexpr float CurlFrequencyScale = 100.f;
	constexpr float CurlAnimSpeedScale = 10.f;
	constexpr float CurlStrengthScale = 0.005f;

	/* Control Point - Dipole */
	constexpr float DipoleStrengthScale = 0.05f;

	/* Emitter */
	constexpr float EmitterNoiseFrequencyMultiplier = 10.f;
	constexpr float EmitterNoiseSpeedScale = 1.f;
}

class FTextureResource;

UCLASS(meta = (PrioritizeCategories = "Aurora"), HideCategories = (HideCategory, Replication, Networking, Input, Cooking, Collision, Physics))
class VOLUMETRICAURORA_API AVolumetricAurora : public AActor
{
	GENERATED_BODY()

	// Allow Details Customization to access private UpdateMaterialTarget()
	friend class FVolumetricAuroraDetailsCustomization;

public:
	AVolumetricAurora();

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostActorCreated() override;

	virtual void PostLoad() override;
#endif


public:
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	void FlowTick(float DeltaTime);

public:

	UPROPERTY(VisibleAnywhere, Category = "Aurora", Instanced, meta = (DisplayName = "Preset Details", NoClear, DisallowNull))
	TObjectPtr<UAuroraPresetBase> TargetAurora;

	// OriginalPreset for Target
	UPROPERTY()
	TObjectPtr<UAuroraPresetBase> SourcePreset;

	// === Time Control ===
	float AuroraAccumulatedTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|Time")
	bool bAuroraPlaying = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|Time", meta = (UIMin = "0.0", UIMax = "10.0", Delta = "0.01"))
	float AuroraTimeScale = 1.0f;

	float FlowSimulationAccumulatedTime = 0.0f;

	UFUNCTION()
	void UpdateMaterialTimeParameter();

#if WITH_EDITOR
	UFUNCTION()
	void ResetFlowSimulation();

	/**
	 * @brief Capture current flow simulation state as checkpoint
	 * Stores current FrontBuffer and simulation time to preset
	 */
	UFUNCTION()
	void CaptureFlowSimulationCheckpoint();
#endif

	// ========================================================================
	// Blueprint Control Functions
	// ========================================================================

	/**
	 * @brief Dynamically change aurora preset at runtime
	 * @param NewPreset Aurora preset to apply
	 * @return true if successfully applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora|Control")
	bool SetAuroraPreset(UAuroraPresetBase* NewPreset);

	/**
	 * @brief Get AuroraPresetBase reference that aurora actor is using
	 * @return AuroraPresetBase reference that aurora actor is using
	 */
	UFUNCTION(BlueprintPure, Category = "Aurora|Control")
	UAuroraPresetBase* GetAuroraPreset();
	
	/**
	 * @brief Show/hide aurora rendering
	 * @param bEnabled true to show, false to hide
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora|Control")
	void SetAuroraEnabled(bool bEnabled);

	/**
	 * @brief Get aurora visibility state
	 */
	UFUNCTION(BlueprintPure, Category = "Aurora|Control")
	bool GetAuroraEnabled() const;
	
	/**
	 * @brief Toggle aurora visibility
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora|Control")
	void ToggleAuroraEnabled();

	/**
	 * @brief Get original source preset (before duplication)
	 * @return Source preset asset that was applied
	 */
	UFUNCTION(BlueprintPure, Category = "Aurora|Control")
	UAuroraPresetBase* GetSourcePreset() const;

	/**
	 * @brief Check if aurora is using specific preset
	 * @param PresetToCheck Preset asset to campare
	 * @return true if current preset matches the source
	 */
	UFUNCTION(BlueprintPure, Category = "Aurora|Control")
	bool IsUsingPreset(UAuroraPresetBase* PresetToCheck) const;

	/**
	 * @brief Debug: Print aurora state to log
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora|Debug")
	void DebugAuroraState();

	/**
	 * @brief Display aurora debug info on screen (PIE/Game)
	 * Shows real-time aurora state directly on viewport
	 * @param bShowDetailed Show detailed material and preset info
	 */
	UFUNCTION(BlueprintCallable, Category = "Aurora|Debug")
	void DisplayAuroraDebugInfo(bool bShowDetailed = false);

	UPROPERTY(VisibleAnywhere, Category = "HideCategory")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> VolumeBox;

	void ApplyPresetToTarget(UAuroraPresetBase* InPreset);

	UFUNCTION()
	FString GetPluginPath();

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HideCategory")
	class UDFBakerComponentBase* DFBakerComponent;


	// ========================================================================
	// Texture Paint System
	// ========================================================================

	/**
	* @brief Render target for texture painting
	*
	* This render target is bound to the paint proxy material.
	* Paint strokes are rendered directly to this target during mesh paint mode.
	*/
	UPROPERTY(Transient)
	UTextureRenderTarget2D* ElementsRenderTarget = nullptr;

	/**
	* @brief Reference to opened paint window
	* Used to track window lifetime and prevent multiple windows
	*/
	TSharedPtr<class SWindow> EditorPaintWindow;

	/**
	* @brief Reference to painter widget instance
	* Kept alive while paint window is open
	* NOTE: Cannot use UPROPERTY - UUserWidget is in UMG module (Editor-only)
	*/
	TObjectPtr<class UUserWidget> EditorPaintWidgetInstance;

	/**
	 * @brief Reference to interactive preview viewport with orbit camera controls
	 *
	 * This Slate widget provides Material Editor-style camera navigation:
	 * - Left Mouse Drag: Orbit camera around the aurora volume
	 * - Mouse Wheel: Zoom in/out (adjust camera distance)
	 *
	 * The camera always focuses on the center of the VolumeBox, allowing users
	 * to inspect the aurora from any angle while painting elements.
	 */
	TSharedPtr<SAuroraPreviewViewport> EditorPreviewViewport;

	/**
	* @brief Initialize render target for texture painting
	*/
	void InitializeElementsRenderTarget();

	/**
	* @brief Create simple material for texture copy operation
	* @param SourceTexture Texture to copy from
	* @return MaterialInstanceDynamic configured for texture copy
	*/
	UMaterialInstanceDynamic* CreateSimpleMaterialForTextureCopy(UTexture* SourceTexture);

	// ========================================================================
	// Preview Aurora System (for Paint Window)
	// ========================================================================

	/**
	 * @brief Transient package that contains preview world
	 *
	 * Separate package to avoid name conflicts with existing levels.
	 * Created with unique random name in /Temp/ namespace.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UPackage> PreviewPackage;

	/**
	 * @brief Dedicated world for preview aurora rendering
	 *
	 * Separate world instance that isolates preview rendering from Editor World.
	 * This prevents preview actors from cluttering the main editor viewport.
	 *
	 * World lifecycle:
	 * - Created in CreatePreviewAurora()
	 * - Destroyed in DestroyPreviewAurora()
	 * - AddToRoot() prevents garbage collection while active
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWorld> PreviewWorld;

	/**
	 * @brief Week pointer to preview aurora actor spawned for paint window
	 *
	 * Weak pointer to preview aurora actor spawned for paint window
	 *
	 * Weak pointer is used because:
	 * - Preview actor is managed by Preview World
	 * - Prevents dangling pointer if actor is destroyed externally
	 * - Can safely check validity with IsValid()
	 */
	TWeakObjectPtr<AVolumetricAurora> PreviewAuroraActor;

	/**
	 * @brief Scene capture component for rendering preview aurora
	 *
	 * SceneCapture2D captures 3D scene from specitic viewpoint and renders to RenderTarget.
	 * Similar to adding an extra camera that outputs to texture instead of screen.
	 */
	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> PreviewSceneCapture;

	/**
	 * @brief Render target that receives scene capture output
	 *
	 * This texture will be displayed in the paint window's preview panel.
	 * Resolution: 1024x1024 (adjustable for quality vs performance)
	 */
	UPROPERTY(Transient)
	UTextureRenderTarget2D* PreviewCaptureTarget = nullptr;

	/**
	 * @brief Create and initialize preview aurora actor for paint window
	 *
	 * Workflow:
	 * 1. Spawn new VolumetricAurora in Editor World at fixed position
	 * 2. Duplicate current Preset settings to preview actor
	 * 3. Connect ElementsRenderTarget to preview's AuroraElementsMap
	 * 4. Hide preview actor from viewport (only visible to SceneCapture)
	 *
	 * @return Pointer to created preview actor, nullptr on failure
	 */
	AVolumetricAurora* CreatePreviewAurora();

	/**
	 * @brief Create and configure scene capture component for previeew rendering
	 *
	 * Sets up:
	 * - Top-down camera view of preview aurora
	 * - Show-only-list filtering (renders only preview actor)
	 * - Manual capture mode (on-demand, not every frame)
	 */
	void CreatePreviewSceneCapture();

	/**
	 * @brief Update preview aurora with current settings and trigger capture
	 *
	 * Call this after:
	 * - Canvas drawing (ElementsRenderTarget modified)
	 * - Preset parameter changes
	 * - Control point modifications
	 */
	void UpdatePreviewAurora();

	/**
	 * @brief Destroy preview aurora and cleanup all preview-related resources
	 *
	 * Called when:
	 * - paint window is closed
	 * - User cancels painting
	 *
	 * Safely handles already-destroyed actors via weak pointer check.
	 */
	void DestroyPreviewAurora();

#endif
private:

	// Transient: Not saved to disk
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AuroraMaterialDynamic = nullptr;

	UPROPERTY()
	TObjectPtr<UAuroraPresetBase> DefaultAurora = nullptr;

	/** Flag to force simulation reset (triggered by manual reset button) */
	bool bForceResetSimulation = false;

	/** Flag indicating control points need GPU buffer update */
	bool bControlPointsDirty = true;

	/** Flag indicating aurora elements map needs update */
	bool bAuroraElementsMapDirty = true;

	/** Track previous aurora elements map to detect content changes */
	UPROPERTY()
	UTexture* PreviousAuroraElementsMap = nullptr;

	/** Previous aurora elements resource pointer for change detection */
	FTextureResource* PreviousAuroraElementsResource = nullptr;

	/** Control points type info */
	TArray<FControlPointInfoGPU> ControlPointsInfo;

	/** Single force control points info */
	TArray<FSingleForceControlPointGPU> SingleForceControlPoints;

	/** Double force control points info */
	TArray<FDoubleForceControlPointGPU> DoubleForceControlPoints;

	/** Triple force control points info */
	TArray<FTripleForceControlPointGPU> TripleForceControlPoints;

	/** Dipole control points info */
	TArray<FDipoleControlPointGPU> DipoleControlPoints;

	/** Curl control points info */
	TArray<FCurlControlPointGPU> CurlControlPoints;

	/** Warp control points info */
	TArray<FWarpControlPointGPU> WarpControlPoints;

#if WITH_EDITORONLY_DATA
	/** Masked billboard base material loaded from path */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ControlPointMaskedBaseMat = nullptr;
#endif

private:
	/** Execute aurora flow simulation compute shader pass */
	void SimulateAuroraPass(UPotentialFlowAuroraPreset* FlowPreset, float DeltaTime);

	/** Bake obstacle distance field using Jump Flooding Algorithm */
	void BakeDistanceMapToRenderTarget(UPotentialFlowAuroraPreset* FlowPreset);

#if WITH_EDITOR
	void RenderControlPointsDebug() const;
#endif

	float MapSize = 50000.0f;

	// Preset folder path cache
	UPROPERTY()
	FString PresetFolder;

	// Plugin path cache
	UPROPERTY()
	FString PluginPath;

	UFUNCTION(BlueprintCallable, Category = "Aurora|Control")
	void UpdateMaterialTarget();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UBillboardComponent* SpriteComponent;
#endif
};
