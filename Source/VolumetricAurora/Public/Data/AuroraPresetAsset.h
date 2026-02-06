// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "Types/AuroraFlowElement.h"
#include "AuroraPresetAsset.generated.h"

UENUM(BlueprintType)
enum class EEdgeFadeMode : uint8
{
	/** 중심에서 밖으로 원형으로 페이드 아웃 (거리 기반) */
	Radial      UMETA(DisplayName = "Radial"),
	/** 박스 형태(X, Y축 각각의 거리)로 페이드 아웃 */
	Box         UMETA(DisplayName = "Box")
};

/**
 * Texture resolution options for simulation
 * Higher resolutions provide more detail but increase GPU cost
 */
UENUM(BlueprintType)
enum class ETextureResolution : uint8
{
	/** Low resolution for optimal performance. Suitable for soft, diffuse aurora effects. */
	Res512		UMETA(DisplayName = "512"),
	/** Balanced resolution providing a good trade-off between detail and performance. */
	Res1024		UMETA(DisplayName = "1024"),
	/** High resolution for sharp, detailed aurora curtains at a higher GPU cost. */
	Res2048		UMETA(DisplayName = "2048")
};

/**
 * Convert ETextureResolution enum to integer pixel dimension
 * @param Resolution Enum resolution value
 * @return Pixel dimension (e.g., 512, 1024, 2048)
 */
inline int32 GetResolutionValue(ETextureResolution Resolution)
{
	switch (Resolution)
	{
	case ETextureResolution::Res512:  return 512;
	case ETextureResolution::Res1024: return 1024;
	case ETextureResolution::Res2048: return 2048;
	default: return 1024;
	}
}

class UTexture;
class UTexture2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class VOLUMETRICAURORA_API UAuroraPresetBase : public UDataAsset
{
	GENERATED_BODY()

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
	/**
	 * Texture that defines the aurora's base shape and pattern
	 * Usage varies by preset type:
	 * - Noise: Continuous noise texture for curtain ripples
	 * - Spline: Distance field texture storing the path information
	 * - Flow: Particle emission map defining spawn regions
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 1))
	UTexture* ShapeTexture = nullptr;

	/** Overall brightness of the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 2,
		Delta = "0.1"))
	double Intensity = 10.0f;

	/** Controls how thick or opaque the aurora appears (higher values create denser aurora) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 3,
		Delta = "0.01"))
	double Density = 1.0f;

	/** Animation speed of the aurora movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 4,
		UIMin = "0.0", UIMax = "5.0", Delta = "0.01"))
	float Speed = 1.0f;

	/** Color at the top of the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 5))
	FLinearColor TopColor = FLinearColor(0.22f, 0.26f, 0.5f, 1.0f);
	/** Color at the middle of the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 6))
	FLinearColor MidColor = FLinearColor(0.25f, 0.3f, 0.5f, 1.0f);
	/** Color at the bottom of the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 7))
	FLinearColor BottomColor = FLinearColor(0.23f, 0.65f, 0.66f, 1.0f);

	/** Height range where the middle color appears (X: start height, Y: end height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 8,
		ClampMin = "0.0", ClampMax = "1.0",
		UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	FVector2f MidColorHeight = FVector2f(0.5f, 0.5f);

	/** Altitude of the aurora above ground level (in km) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 9,
		Units = "km", ForceUnits = "km",
		UIMin = "0.0", Delta = "0.01"))
	double Altitude = 1.1f;
	/** Horizontal extent of the aurora area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 10,
		UIMin = "0.0", Delta = "0.1"))
	double AuroraAreaExtent = 50.0f;
	/** Vertical height of the aurora volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 11,
		UIMin = "0.0", Delta = "0.01"))
	double AuroraHeight = 1.2f;

	/** Method for fading out the aurora at the edges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 12))
	EEdgeFadeMode EdgeFadeMode = EEdgeFadeMode::Box;
	/** Softness of the edge fade (0 = sharp cut, 1 = smooth gradual fade) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 13,
		ClampMin = "0.0", ClampMax = "1.0",
		UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	float EdgeFadeSoftness = 0.5f;

	/** How quickly the aurora fades out toward the top (higher values fade faster) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 14,
		ClampMin = "0.01",
		UIMin = "0.01", UIMax = "10.0", Delta = "0.01"))
	float HeightFalloff = 1.0f;

	/** Enable film grain effect for a more organic look */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance", meta = (DisplayPriority = 1))
	bool bEnableFilmGrain = false;
	/** Strength of the film grain noise */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance", meta = (DisplayPriority = 2,
		EditCondition = "bEnableFilmGrain",
		ClampMin = "0.0",
		UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	float FilmGrainIntensity = 0.15f;

	/** Use straight alpha blending for the color palette */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Color", meta = (DisplayPriority = 1))
	bool bUseStraightAlphaPalette = false;

	/** Strength of the color displacement noise, creating a shimmering ripple effect across color bands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Color", meta = (DisplayPriority = 2,
		ClampMin = "0.0", ClampMax = "0.25",
		UIMin = "0.0", UIMax = "0.25", Delta = "0.001"))
	float ColorShiftStrength = 0.05f;

	/** Enable additional color tinting on dense structural regions of the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Color", meta = (DisplayPriority = 3))
	bool bEnableStructureColoring = false;
	/** Tint color applied to the outer edges of dense structures */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Color", meta = (DisplayPriority = 4,
		HideAlphaChannel, EditCondition = "bEnableStructureColoring"))
	FLinearColor SoftTint = FLinearColor(0.10f, 0.85f, 0.35f, 1.0f);
	/** Tint color applied to the densest core of structures */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Color", meta = (DisplayPriority = 5,
		HideAlphaChannel, EditCondition = "bEnableStructureColoring"))
	FLinearColor CoreTint = FLinearColor(0.20f, 0.75f, 1.00f, 1.0f);

	/** Pivot point for tone redistribution (brighter regions get brighter, darker get darker) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Tone", meta = (DisplayPriority = 1,
		ClampMin = "0.001",
		UIMin = "0.001", UIMax = "0.05", Delta = "0.001"))
	float EmissiveTonePivot = 0.02f;
	/** Contrast strength of the aurora emission (1.0 = neutral, higher = more contrast) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Tone", meta = (DisplayPriority = 2,
		ClampMin = "0.0",
		UIMin = "0.0", UIMax = "2.0", Delta = "0.01"))
	float EmissiveContrast = 1.0f;
	/** Color saturation of the aurora (0 = grayscale, 1 = natural, >1 = oversaturated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Tone", meta = (DisplayPriority = 3,
		ClampMin = "0.0",
		UIMin = "0.0", UIMax = "2.0", Delta = "0.01"))
	float EmissiveSaturation = 1.0f;

	/** Enable enhanced brightness on dense structural regions of the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Structure", meta = (DisplayPriority = 1,
		ClampMin = "0.0", ClampMax = "1.0",
		UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	bool bEnableEmissiveStructure = false;
	/** Density threshold for determining what counts as a bright structure */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Structure", meta = (DisplayPriority = 2,
		EditCondition = "bEnableEmissiveStructure",
		ClampMin = "0.0",
		UIMin = "0.0", UIMax = "0.05", Delta = "0.001"))
	float EmissiveStructurePivot = 0.02f;
	/** Selectivity of structure brightness (higher = only well-defined bands glow, lower = more diffuse glow) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Appearance|Structure", meta = (DisplayPriority = 3,
		EditCondition = "bEnableEmissiveStructure",
		ClampMin = "0.001",
		UIMin = "0.001", UIMax = "3.0", Delta = "0.01"))
	float EmissiveStructureSelectivity = 1.0f;

	bool IsIdentical(UAuroraPresetBase* Other);

	virtual void CopyFrom(UAuroraPresetBase* Source);

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance);
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (PrioritizeCategories = "Aurora|PresetDetails|NoiseSettings Aurora|PresetDetails|Appearance"))
class VOLUMETRICAURORA_API UNoiseAuroraPreset : public UAuroraPresetBase
{
	GENERATED_BODY()

public:
	UNoiseAuroraPreset();
	/** Frequency of the aurora curtain folds (higher values create tighter, more detailed ripples) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|NoiseSettings", meta = (DisplayPriority = 1,
		UIMin = "0.01", UIMax = "2.0", Delta = "0.01"))
	float ShapeFrequency = 1.0f;
	/** Speed and direction the aurora drifts across the sky */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|NoiseSettings", meta = (DisplayPriority = 2,
		UIMin = "-0.1", UIMax = "0.1", Delta = "0.001"))
	FVector2f ScrollVelocity = FVector2f(0.001f, 0.001f);

	/** Smoothness of the curtain folds (lower = sharp thin edges, higher = soft billowy shapes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|NoiseSettings", meta = (DisplayPriority = 4,
		UIMin = "0.01", UIMax = "1.0", Delta = "0.01"))
	float Smoothness = 0.2f;
	/** Frequency of the mask pattern that creates gaps and breaks in the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|NoiseSettings", meta = (DisplayPriority = 5,
		UIMin = "0.01", UIMax = "2.0", Delta = "0.01"))
	float MaskFrequency = 1.0f;
	/** Speed the mask pattern moves across the aurora */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|NoiseSettings", meta = (DisplayPriority = 6,
		UIMin = "-0.1", UIMax = "0.1", Delta = "0.001"))
	FVector2D MaskScrollVelocity = FVector2D(0.001f, 0.001f);
	/** How much the mask can hide the aurora (1.0 = can completely hide, 0 = no masking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|NoiseSettings", meta = (DisplayPriority = 7,
		UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	float MaskOpacity = 0.9f;


	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance) override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (PrioritizeCategories = "Aurora|PresetDetails|SplineSettings Aurora|PresetDetails|Appearance"))
class VOLUMETRICAURORA_API USplineAuroraPreset : public UAuroraPresetBase
{
	GENERATED_BODY()
public:

	USplineAuroraPreset();

	/** Amount of vertical streaking applied to the aurora (values above 0.2 may cause extreme distortion) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|SplineSettings", meta = (DisplayPriority = 1,
		UIMin = "0.0", UIMax = "0.2", Delta = "0.001", Logarithmic = "true"))
	float Distortion = 0.04f;

	/** How distortion fades with height (0 = uniform, higher = less distortion at the top) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|SplineSettings", meta = (DisplayPriority = 2,
		UIMin = "0.0", UIMax = "10.0", Delta = "0.01"))
	float DistortionHeightFalloff = 0.0f;

	/** Thickness of the aurora along the spline path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|SplineSettings", meta = (DisplayPriority = 3,
		UIMin = "0.0", UIMax = "1000.0", Delta = "0.1"))
	float Thickness = 350.0f;

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance) override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (PrioritizeCategories = "Aurora|PresetDetails|FlowSettings Aurora|PresetDetails|Appearance"))
class VOLUMETRICAURORA_API UPotentialFlowAuroraPreset : public UAuroraPresetBase
{
	GENERATED_BODY()
public:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	// ========================================================================
	// Render Targets
	// ========================================================================

	/** Ping-pong buffer: write target for current simulation step */
	UPROPERTY()
	UTextureRenderTarget2D* FrontBuffer = nullptr;

	/** Ping-pong buffer: read source for current simulation step */
	UPROPERTY()
	UTextureRenderTarget2D* BackBuffer = nullptr;

	/** Baked obstacle distance field (RGBA: Normal.xy, Distance, Mask) */
	UPROPERTY()
	UTextureRenderTarget2D* ObstacleMap = nullptr;

	/** Display buffer for external use (snapshot before ping-pong swap) */
	UPROPERTY()
	UTextureRenderTarget2D* DisplayBuffer = nullptr;

	/** Show visual markers for control points in the editor */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 1))
	bool bDisplayControlPoints = true;

	/** Show the range circles for control point attenuation */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 2,
		EditCondition = "bDisplayControlPoints"))
	bool bDisplayAttenuationRange = true;

	/** Size of the control point markers */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 3,
		EditCondition = "bDisplayControlPoints"))
	float DisplaySize = 5.f;

	/** Z-axis position offset for displaying control point markers */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 4,
		EditCondition = "bDisplayControlPoints"))
	float DisplayZPos = 0.f;

	// ========================================================================
	// Simulation Parameters
	// ========================================================================

	/** Resolution of the simulation texture (higher = more detail but more GPU cost) */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 5))
	ETextureResolution SimulationResolution = ETextureResolution::Res1024;

	/** Reset simulation when control points or shape texture are changed */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 6))
	bool bResetSimulationOnElementChange = true;

	/** Base flow direction and speed of the aurora */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 7,
		ClampMin = "-1.0", ClampMax = "1.0",
		UIMin = "-1.0", UIMax = "1.0"))
	FVector2D BaseFlow = FVector2D(0.1, 0.0);

	// ========================================================================
	// Control Points
	// ========================================================================

	/** Control points that influence the flow and behavior of the aurora */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 8,
		TitleProperty = "Type"))
	TArray<FAuroraFlowElement> ControlPoints;

	// ========================================================================
	// Aurora Elements
	// ========================================================================

	/**
	 * @brief Captured simulation state texture
	 * Stores flow simulation snapshot for resuming from specific point
	 * Format: PF_FloatRGBA (HDR), saved as permanent asset
	 */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 8))
	TObjectPtr<UTexture2D> SimulationCheckpointTexture = nullptr;

	/**
	 * @brief Simulation time when checkpoint was captured
	 * Used to restore accurate time state during checkpoint restoration
	 */
	UPROPERTY(VisibleAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 9))
	float SimulationCheckpointTime = 0.0f;

	/**
	 * @brief Resolution of captured checkpoint texture
	 * Used for validation during restore to ensure dimension match
	 */
	UPROPERTY(VisibleAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 10))
	int32 SimulationCheckpointResolution = 0;

	// ========================================================================
	// Emitter Noise Parameters
	// ========================================================================

	/** Detail level of the emission pattern (higher = more varied emission) */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 11,
		ClampMin = "0.01", ClampMax = "1.0",
		UIMin = "0.01", UIMax = "1.0"))
	float EmitterNoiseFrequency = 0.01f;

	/** Animation speed of the emission pattern variation */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 12,
		ClampMin = "0.01", ClampMax = "1.0",
		UIMin = "0.01", UIMax = "1.0"))
	float EmitterNoiseSpeed = 0.05f;

	/** Strength of the emission pattern variation */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|FlowSettings", meta = (DisplayPriority = 13,
		ClampMin = "0.0", ClampMax = "1.0",
		UIMin = "0.0", UIMax = "1.0"))
	float EmitterNoiseStrength = 0.2f;

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance) override;

	/**
	 * @brief Capture current simulation state to checkpoint texture
	 * @param CurrentSimulationTime  Current accumulated simulation time
	 */
#if WITH_EDITOR
	void CaptureSimulationCheckpoint(FString TargetAuroraName, float CurrentSimulationTime);

#endif
	/**
	 * @brief Check if checkpoint texture exists and can be restored
	 * @return true if valid checkpoint exists
	 */
	bool HasValidCheckpoint() const;
};