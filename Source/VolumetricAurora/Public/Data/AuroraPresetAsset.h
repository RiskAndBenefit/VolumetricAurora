// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "Types/AuroraFlowElement.h"
#include "AuroraPresetAsset.generated.h"

UENUM(BlueprintType)
enum class EFadeType : uint8
{
	Radial      UMETA(DisplayName = "Radial"),
	Box         UMETA(DisplayName = "Box")
};

/**
 * Texture resolution options for simulation
 * Higher resolutions provide more detail but increase GPU cost
 */
UENUM(BlueprintType)
enum class ETextureResolution : uint8
{
	Res512		UMETA(DisplayName = "512"),
	Res1024		UMETA(DisplayName = "1024"),
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
	default: return 2048;
	}
}

class UTexture;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 1))
	UTexture* NoiseTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 2, UIMin = "0.0", Units = "km", ForceUnits = "km"))
	float Altitude = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 3, UIMin = "0.0"))
	float AuroraAreaExtent = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 4, UIMin = "0.0"))
	float AuroraHeight = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 5))
	float Intensity = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 6))
	FVector2f Speed = FVector2f(0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 7))
	float BaseDensity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 8))
	float BaseScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 9, HideAlphaChannel))
	FLinearColor TopColor = FLinearColor(0.22f, 0.26f, 0.5f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 10, HideAlphaChannel))
	FLinearColor MidColor = FLinearColor(0.25f, 0.3f, 0.5f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 11, HideAlphaChannel))
	FLinearColor BottomColor = FLinearColor(0.23f, 0.65f, 0.66f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 12, ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	FVector2f MidColorHeight = FVector2f(0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 13))
	EFadeType FadeType = EFadeType::Box;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 14, UIMin = "0.0", UIMax = "1.0"))
	float FadeStartRatio = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails", meta = (DisplayPriority = 15))
	float HeightFalloff = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look")
	bool bEnableFilmGrain = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look", meta = (EditCondition = "bEnableFilmGrain", ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	float FilmGrainIntensity = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Tone", meta = (DisplayPriority = 1, ClampMin = "0.001", UIMin = "0.001", UIMax = "0.05", Delta = "0.001"))
	float EmissiveTonePivot = 0.02f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Tone", meta = (DisplayPriority = 2, ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Delta = "0.01"))
	float EmissiveContrast = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Tone", meta = (DisplayPriority = 2, ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Delta = "0.01"))
	float EmissiveSaturation = 1.0f;

	bool IsIdentical(UAuroraPresetBase* Other);

	virtual void CopyFrom(UAuroraPresetBase* Source);

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance);
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class VOLUMETRICAURORA_API UNoiseAuroraPreset : public UAuroraPresetBase
{
	GENERATED_BODY()
public:

	UNoiseAuroraPreset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced")
	float Activity = 0.005f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced")
	float Smoothness = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced")
	float MaskScaleMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced")
	FVector2D MaskSpeedMultiplier = FVector2D(1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced", meta = (UIMin = "0.0", UIMax = "1.0"))
	float MaskOpacity = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Color", meta = (DisplayPriority = 1))
	bool bUseStructureColoring = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Color", meta = (DisplayPriority = 2, HideAlphaChannel))
	FLinearColor SoftTint = FLinearColor(0.10f, 0.85f, 0.35f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Color", meta = (DisplayPriority = 3, HideAlphaChannel))
	FLinearColor CoreTint = FLinearColor(0.20f, 0.75f, 1.00f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Structure", meta = (DisplayPriority = 1, ClampMin = "0.0", UIMin = "0.0", UIMax = "0.05", Delta = "0.001"))
	float EmissiveStructurePivot = 0.02f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Structure", meta = (DisplayPriority = 2, ClampMin = "0.001", UIMin = "0.001", UIMax = "3.0", Delta = "0.01"))
	float EmissiveStructureSelectivity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Look|Structure", meta = (DisplayPriority = 3, ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", Delta = "0.01"))
	float EmissiveStructureAmount = 0.0f;

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance) override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class VOLUMETRICAURORA_API USplineAuroraPreset : public UAuroraPresetBase
{
	GENERATED_BODY()
public:

	USplineAuroraPreset();

	// The amount of vertical streaking applied to the aurora
	// Values above 0.2 may cause extreme distortion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced", meta = (
		UIMin = "0.0",
		UIMax = "0.2",
		Logarithmic = "true"
		))
	float Distortion = 0.04f;

	// Adjust how fast the vertical streaks jitter and shift
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced", meta = (
		UIMin = "-1.0",
		UIMax = "1.0",
		Logarithmic = "true"
		))
	float DistortionSpeed = 0.02f;

	// Controls the distortion fade-off based on height. 
	// At 0, distortion is uniform, and Higher values reduce distortion at the top
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced", meta = (
		UIMin = "0.0",
		UIMax = "10.0"
		))
	float DistortionHeightFalloff = 0.0f;

	// Aurora thickness along the spline path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aurora|PresetDetails|Advanced")
	float Thickness = 350.0f;

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance) override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
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

	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	bool bDisplayControlPoints = true;

	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	bool bDisplayAttenuationRange = true;
	
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	float DisplaySize = 5.f;

	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	float DisplayZPos = 0.f;
	
	// ========================================================================
	// Simulation Parameters
	// ========================================================================

	/** Simulation texture resolution */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	ETextureResolution SimulationResolution = ETextureResolution::Res2048;

	/** Whether to reset simulation when flow elements (ControlPoints, AuroraElementsMap) are changed */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	bool bResetSimulationOnElementChange = true;

	/** Base flow velocity (X=horizontal, Y=vertical direction and speed) */
	UPROPERTY(
		EditAnywhere,
		Category = "Aurora|PresetDetails|Flow",
		meta = (
			UIMin = "0.0", UIMax = "2.0",
			ClampMin = "0.0"))
	float FlowTimeScale = 1.f;

	/** Base flow velocity (X=horizontal, Y=vertical direction and speed) */
	UPROPERTY(
		EditAnywhere,
		Category = "Aurora|PresetDetails|Flow",
		meta = (
			ClampMin = "-1.0", ClampMax = "1.0",
			UIMin = "-1.0", UIMax = "1.0"))
	FVector2D BaseFlow = FVector2D(0.1, 0.0);

	// ========================================================================
	// Control Points
	// ========================================================================

	/** Flow field control points (source, sink, vortex, spiral, curl, dipole) */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow", meta = (TitleProperty = "Type"))
	TArray<FAuroraFlowElement> ControlPoints;

	// ========================================================================
	// Aurora Elements
	// ========================================================================

	/** Texture map defining emitter regions (R channel) and fade zones (G channel) */
	UPROPERTY(EditAnywhere, Category = "Aurora|PresetDetails|Flow")
	UTexture* AuroraElementsMap = nullptr;

	// ========================================================================
	// Emitter Noise Parameters
	// ========================================================================

	/** Spatial frequency of emitter noise (higher = more detail) */
	UPROPERTY(
		EditAnywhere,
		Category = "Aurora|PresetDetails|Flow",
		meta = (
			ClampMin = "0.01", ClampMax = "1.0",
			UIMin = "0.01", UIMax = "1.0"))
	float EmitterNoiseFrequency = 0.01f;

	/** Animation speed of emitter noise pattern */
	UPROPERTY(
		EditAnywhere,
		Category = "Aurora|PresetDetails|Flow",
		meta = (
			ClampMin = "0.01", ClampMax = "1.0",
			UIMin = "0.01", UIMax = "1.0"))
	float EmitterNoiseSpeed = 0.05f;

	/** Strength/amplitude of emitter noise variation */
	UPROPERTY(
		EditAnywhere,
		Category = "Aurora|PresetDetails|Flow",
		meta = (
			ClampMin = "0.0", ClampMax = "1.0",
			UIMin = "0.0", UIMax = "1.0"))
	float EmitterNoiseStrength = 0.2f;

	virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance) override;
};