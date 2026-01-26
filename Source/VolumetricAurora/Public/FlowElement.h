// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlowElement.generated.h"

/**
 * Control point type enumeration
 * Defines available flow field primitives
 */
UENUM(BlueprintType)
enum class EControlPointType : uint8
{
	None			UMETA(DisplayName = "None"),
	Source			UMETA(DisplayName = "Source"),			// Radial source (outward)
	Sink			UMETA(DisplayName = "Sink"),			// Radial sink (inward) with attenuation
	Vortex			UMETA(DisplayName = "Vortex"),			// Rotational flow
	Spiral			UMETA(DisplayName = "Spiral"),			// Sink + rotation
	Dipole			UMETA(DisplayName = "Dipole"),			// Directional flow (source+sink pair)
	Curl			UMETA(DisplayName = "Curl"),			// Curl noise field
	Warp			UMETA(DisplayName = "Warp"),			// Domain warping noise field
	Emitter			UMETA(DisplayName = "Emitter"),		// Emit particle
	Attenuator		UMETA(DisplayName = "Attenuator"),		// Attenuate particle
};

UENUM(BlueprintType)
enum class EControlPointRange : uint8
{
	Local			UMETA(DisplayName = "Local"),
	Global			UMETA(DisplayName = "Global")
};

/**
 * Flow field control point configuration
 * CPU-side structure that gets converted to FControlPointGPU for shader use
 *
 * NOTE:
 * - Existing tooltips are preserved: do not remove /** ... *\/ comments.
 * - AttenuationStart/End and Exponent are only visible when Range == Local.
 */
USTRUCT(BlueprintType)
struct FFlowElement
{
	GENERATED_BODY()

	/** Control point type (determines which fields are active) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuroraControlPoint")
	EControlPointType Type = EControlPointType::None;

	/** Whether this control point applies globally (ignores distance-based attenuation parameters) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type != EControlPointType::None",
			EditConditionHides))
	EControlPointRange Range = EControlPointRange::Global;

	/** Show advanced parameters for Curl/Warp noise types */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl || Type == EControlPointType::Warp",
			EditConditionHides))
	bool bShowAdvanced = false;

	// ========================================================================
	// Debug Parameter
	// ========================================================================

	/** Whether to visualize the control point's location */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "DebugVisualization",
		meta = (
			EditCondition = "Range != EControlPointRange::Global",
			EditConditionHides))
	bool bDisplayControlPoint = true;
	
	/** Whether to visualize the control point's attenuation range */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "DebugVisualization",
		meta = (
			EditCondition = "Range != EControlPointRange::Global",
			EditConditionHides))
	bool bDisplayAttenuationRange = true;
	
	// ========================================================================
	// Position-based Control Points (Source, Sink, Vortex, Spiral, Dipole)
	// ========================================================================

	/** Control point center position in UV space [0, 1] */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type != EControlPointType::None",
			EditConditionHides,
			UIMin = "-0.5", UIMax = "1.5"))
	FVector2D Position = FVector2D(0.0, 0.0);

	// ========================================================================
	// Radial (Source / Sink / Spiral)
	// ========================================================================

	/** Radial flow strength (controls outward/inward flow intensity) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral",
			EditConditionHides,
			UIMin = "-1.0", UIMax = "1.0"))
	float RadialStrength = 0.f;

	/** Distance at which radial attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float RadialAttenuationStart = 0.0f;

	/** Distance at which radial influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float RadialAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for radial influence (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float RadialExponent = 2.0f;

	// ========================================================================
	// Source Parameters
	// ========================================================================

	/** Density emission strength for Source control points */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Source || Type == EControlPointType::Emitter",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0",
			ClampMin = "0.0"))
	float EmissionStrength = 0.f;

	/** Distance at which emission attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Emitter) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float EmissionAttenuationStart = 0.0f;

	/** Distance at which emission influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Emitter) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float EmissionAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for emission influence (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Emitter) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float EmissionExponent = 2.0f;

	// ========================================================================
	// Vortex / Spiral Parameters
	// ========================================================================

	/** Rotational strength (positive=CW, negative=CCW) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Vortex || Type == EControlPointType::Spiral",
			EditConditionHides,
			UIMin = "-1.0", UIMax = "1.0"))
	float RotationStrength = 0.f;

	/** Distance at which rotation attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Vortex || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float RotationAttenuationStart = 0.0f;

	/** Distance at which rotation influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Vortex || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float RotationAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for rotation influence (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Vortex || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float RotationExponent = 2.0f;

	/** Density fade strength (higher value = faster density decay) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0",
			ClampMin = "0.0"))
	float FadeStrength = 0.f;

	/** Distance at which density fade attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float FadeAttenuationStart = 0.0f;

	/** Distance at which density fade influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float FadeAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for density fade (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float FadeExponent = 2.0f;

	// ========================================================================
	// Dipole Parameters
	// ========================================================================

	/** Dipole axis direction (normalized automatically) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole",
			EditConditionHides,
			UIMin = "-1.0", UIMax = "1.0"))
	FVector2D DipoleDirection = FVector2D(1.0f, 0.0f);

	/** Dipole moment strength (scales flow intensity) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0",
			ClampMin = "0.0"))
	float DipoleStrength = 0.1f;

	/** Distance at which dipole attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float DipoleAttenuationStart = 0.0f;

	/** Distance at which dipole influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float DipoleAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for dipole influence (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float DipoleExponent = 2.0f;

	// ========================================================================
	// Curl Noise Parameters (Curl)
	// ========================================================================
	
	/** Spatial frequency of curl noise (higher = smaller, more detailed flow patterns) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0"))
	float CurlFrequency = 0.5f;

	/** Animation speed of curl noise pattern */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0"))
	float CurlAnimationSpeed = 0.5f;

	/** Detail layers for curl noise (higher = more complex patterns) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "1", UIMax = "5",
			ClampMin = "1", ClampMax = "5"))
	int32 CurlOctaves = 3;

	/** Frequency increase per detail layer (higher = sharper detail steps) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && CurlOctaves > 1 && bShowAdvanced",
			EditConditionHides,
			UIMin = "1.0", UIMax = "3.0",
			ClampMin = "1.0"))
	float CurlLacunarity = 2.0f;

	/** Amplitude multiplier per octave (typically 0.5) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && CurlOctaves > 1 && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.3", UIMax = "1.0",
			ClampMin = "0.3"))
	float CurlGain = 0.5f;

	/** Overall intensity scale of curl noise */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && CurlOctaves > 1 && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.5", UIMax = "2.0",
			ClampMin = "0.5"))
	float CurlAmplitude = 1.f;

	/** Final flow strength multiplier for curl field */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0",
			ClampMin = "0.0"))
	float CurlStrength = 0.5f;

	/** Distance at which curl attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float CurlAttenuationStart = 0.0f;

	/** Distance at which curl influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float CurlAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for curl noise influence (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float CurlExponent = 2.0f;

	// ========================================================================
	// Domain Warping Parameters (Warp)
	// ========================================================================

	/** Number of iterative domain warp passes (more = more distortion) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "1", UIMax = "5",
			ClampMin = "1", ClampMax = "5"))
	int32 WarpIterations = 2;

	/** Initial warp displacement strength */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0", UIMax = "2.0",
			ClampMin = "0.0"))
	float WarpDisplacement = 1.0f;

	/** Strength decay per warp iteration */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0",
			ClampMin = "0.0"))
	float WarpFalloff = 0.5f;

	/** Output contrast adjustment (higher = more contrast) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.1", UIMax = "3.0",
			ClampMin = "0.1"))
	float WarpContrast = 1.0f;

	/** Animation displacement magnitude */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.0", UIMax = "2.0",
			ClampMin = "0.0"))
	float WarpAnimAmplitude = 0.5f;

	/** Animation speed multiplier */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0", UIMax = "2.0",
			ClampMin = "0.0"))
	float WarpAnimationSpeed = 0.5f;

	/** X-axis warping noise scale */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.1", UIMax = "4.0",
			ClampMin = "0.1"))
	float WarpXScale = 1.0f;

	/** Y-axis warping noise scale */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.1", UIMax = "4.0",
			ClampMin = "0.1"))
	float WarpYScale = 1.0f;

	/** Result noise sample scale */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.1", UIMax = "4.0",
			ClampMin = "0.1"))
	float WarpNoiseScale = 1.0f;

	/** X-axis noise sampling offset */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides))
	FVector2D WarpXOffset = FVector2D(0.0f, 0.0f);

	/** Y-axis noise sampling offset */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides))
	FVector2D WarpYOffset = FVector2D(100.0f, 0.0f);

	/** Result noise sampling offset */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && bShowAdvanced",
			EditConditionHides))
	FVector2D WarpNoiseOffset = FVector2D(0.0f, 100.0f);

	/** FBM octave count for warp noise */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "1", UIMax = "5",
			ClampMin = "1", ClampMax = "5"))
	int32 WarpOctaves = 3;

	/** Frequency multiplier per octave */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && WarpOctaves > 1 && bShowAdvanced",
			EditConditionHides,
			UIMin = "1.0", UIMax = "3.0",
			ClampMin = "1.0"))
	float WarpLacunarity = 2.0f;

	/** Amplitude multiplier per octave */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && WarpOctaves > 1 && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.3", UIMax = "1.0",
			ClampMin = "0.3"))
	float WarpGain = 0.5f;

	/** Initial FBM amplitude */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && WarpOctaves > 1 && bShowAdvanced",
			EditConditionHides,
			UIMin = "0.5", UIMax = "2.0",
			ClampMin = "0.5"))
	float WarpInitialAmplitude = 1.0f;

	/** Final flow strength multiplier for warp field */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.0",
			ClampMin = "0.0"))
	float WarpFlowStrength = 0.5f;

	/** Distance at which warp attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float WarpAttenuationStart = 0.0f;

	/** Distance at which warp influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0", UIMax = "1.5",
			ClampMin = "0.0"))
	float WarpAttenuationEnd = 0.5f;

	/** Distance-based falloff exponent for warp influence (higher = sharper attenuation) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0", UIMax = "8.0",
			ClampMin = "0.001"))
	float WarpExponent = 2.0f;
};
