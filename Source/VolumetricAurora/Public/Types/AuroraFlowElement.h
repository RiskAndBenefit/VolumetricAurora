// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AuroraFlowElement.generated.h"

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
	Emitter			UMETA(DisplayName = "Emitter"),			// Emit particle
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
struct FAuroraFlowElement
{
	GENERATED_BODY()

	/** Type of control point (determines which fields are active) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			ToolTip = "Type of control point (determines which fields are active)"
		)
	)
	EControlPointType Type = EControlPointType::None;

	/** Whether this control point affects the entire aurora (Global) or only a local region (Local) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type != EControlPointType::None",
			EditConditionHides,
			ToolTip = "Whether this control point affects the entire aurora (Global) or only a local region (Local)"
		)
	)
	EControlPointRange Range = EControlPointRange::Global;

	// ========================================================================
	// Position-based Control Points (Source, Sink, Vortex, Spiral, Dipole)
	// ========================================================================

	/** Position of the control point (0.5, 0.5 = center) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type != EControlPointType::None && !(Type == EControlPointType::Curl && Range == EControlPointRange::Global) && !(Type == EControlPointType::Warp && Range == EControlPointRange::Global)",
			EditConditionHides,
			UIMin = "-0.5",
			UIMax = "1.5",
			ToolTip = "Position of the control point (0.5, 0.5 = center)"
		)
	)
	FVector2D Position = FVector2D(0.0, 0.0);

	// ========================================================================
	// Debug Parameter
	// ========================================================================

	/** Show a visual marker for this control point in the editor */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "DebugVisualization",
		meta = (
			EditCondition = "Type != EControlPointType::None && !(Type == EControlPointType::Curl && Range == EControlPointRange::Global) && !(Type == EControlPointType::Warp && Range == EControlPointRange::Global)",
			EditConditionHides,
			ToolTip = "Show a visual marker for this control point in the editor"
		)
	)
	bool bDisplayControlPoint = true;

	/** Show the influence range of this control point in the editor */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "DebugVisualization",
		meta = (
			EditCondition = "Type != EControlPointType::None && Range != EControlPointRange::Global",
			EditConditionHides,
			ToolTip = "Show the influence range of this control point in the editor"
		)
	)
	bool bDisplayAttenuationRange = true;

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
			UIMin = "-1.0",
			UIMax = "1.0",
			ToolTip = "Radial flow strength (controls outward/inward flow intensity)"
		)
	)
	float RadialStrength = 0.f;

	/** Distance at which radial attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which radial attenuation begins (full strength inside)"
		)
	)
	float RadialAttenuationStart = 0.0f;

	/** Distance at which radial influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which radial influence reaches zero"
		)
	)
	float RadialAttenuationEnd = 0.5f;

	/** How sharply the radial effect fades with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Sink || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the radial effect fades with distance (higher = more abrupt fade)"
		)
	)
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
			UIMin = "0.0",
			UIMax = "1.0",
			ClampMin = "0.0",
			ToolTip = "Density emission strength for Source control points"
		)
	)
	float EmissionStrength = 0.f;

	/** Distance at which emission attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Emitter) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which emission attenuation begins (full strength inside)"
		)
	)
	float EmissionAttenuationStart = 0.0f;

	/** Distance at which emission influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Emitter) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which emission influence reaches zero"
		)
	)
	float EmissionAttenuationEnd = 0.5f;

	/** How sharply the emission fades with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Source || Type == EControlPointType::Emitter) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the emission fades with distance (higher = more abrupt fade)"
		)
	)
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
			UIMin = "-1.0",
			UIMax = "1.0",
			ToolTip = "Rotational strength (positive=CW, negative=CCW)"
		)
	)
	float RotationStrength = 0.f;

	/** Distance at which rotation attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Vortex || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which rotation attenuation begins (full strength inside)"
		)
	)
	float RotationAttenuationStart = 0.0f;

	/** Distance at which rotation influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Vortex || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which rotation influence reaches zero"
		)
	)
	float RotationAttenuationEnd = 0.5f;

	/** How sharply the rotation fades with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Vortex || Type == EControlPointType::Spiral) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the rotation fades with distance (higher = more abrupt fade)"
		)
	)
	float RotationExponent = 2.0f;

	/** Density fade strength (higher value = faster density decay) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.0",
			ClampMin = "0.0",
			ToolTip = "Density fade strength (higher value = faster density decay)"
		)
	)
	float FadeStrength = 0.f;

	/** Distance at which density fade attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which density fade attenuation begins (full strength inside)"
		)
	)
	float FadeAttenuationStart = 0.0f;

	/** Distance at which density fade influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which density fade influence reaches zero"
		)
	)
	float FadeAttenuationEnd = 0.5f;

	/** How sharply the density fade occurs with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "(Type == EControlPointType::Sink || Type == EControlPointType::Vortex || Type == EControlPointType::Spiral || Type == EControlPointType::Attenuator) && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the density fade occurs with distance (higher = more abrupt fade)"
		)
	)
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
			UIMin = "-1.0",
			UIMax = "1.0",
			ToolTip = "Dipole axis direction (normalized automatically)"
		)
	)
	FVector2D DipoleDirection = FVector2D(1.0f, 0.0f);

	/** Dipole moment strength (scales flow intensity) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.0",
			ClampMin = "0.0",
			ToolTip = "Dipole moment strength (scales flow intensity)"
		)
	)
	float DipoleStrength = 0.1f;

	/** Distance at which dipole attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which dipole attenuation begins (full strength inside)"
		)
	)
	float DipoleAttenuationStart = 0.0f;

	/** Distance at which dipole influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which dipole influence reaches zero"
		)
	)
	float DipoleAttenuationEnd = 0.5f;

	/** How sharply the dipole effect fades with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Dipole && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the dipole effect fades with distance (higher = more abrupt fade)"
		)
	)
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
			UIMin = "0.0",
			UIMax = "1.0",
			ToolTip = "Spatial frequency of curl noise (higher = smaller, more detailed flow patterns)"
		)
	)
	float CurlFrequency = 0.5f;

	/** Animation speed of curl noise pattern */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.0",
			ToolTip = "Animation speed of curl noise pattern"
		)
	)
	float CurlAnimationSpeed = 0.5f;

	/** Detail layers for curl noise (higher = more complex patterns) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "1",
			UIMax = "5",
			ClampMin = "1",
			ClampMax = "5",
			ToolTip = "Detail layers for curl noise (higher = more complex patterns)"
		)
	)
	int32 CurlOctaves = 3;

	/** How much finer detail each layer adds (higher = more dramatic detail contrast between layers) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && CurlOctaves > 1",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "3.0",
			ClampMin = "1.0",
			ToolTip = "How much finer detail each layer adds (higher = more dramatic detail contrast between layers)"
		)
	)
	float CurlLacunarity = 2.0f;

	/** How much influence each detail layer has (lower = smoother, higher = rougher) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && CurlOctaves > 1",
			EditConditionHides,
			UIMin = "0.3",
			UIMax = "1.0",
			ClampMin = "0.3",
			ToolTip = "How much influence each detail layer has (lower = smoother, higher = rougher)"
		)
	)
	float CurlGain = 0.5f;

	/** Overall intensity scale of curl noise */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && CurlOctaves > 1",
			EditConditionHides,
			UIMin = "0.5",
			UIMax = "2.0",
			ClampMin = "0.5",
			ToolTip = "Overall intensity scale of curl noise"
		)
	)
	float CurlAmplitude = 1.f;

	/** Final flow strength multiplier for curl field */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.0",
			ClampMin = "0.0",
			ToolTip = "Final flow strength multiplier for curl field"
		)
	)
	float CurlStrength = 0.5f;

	/** Distance at which curl attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which curl attenuation begins (full strength inside)"
		)
	)
	float CurlAttenuationStart = 0.0f;

	/** Distance at which curl influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which curl influence reaches zero"
		)
	)
	float CurlAttenuationEnd = 0.5f;

	/** How sharply the curl effect fades with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Curl && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the curl effect fades with distance (higher = more abrupt fade)"
		)
	)
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
			UIMin = "1",
			UIMax = "5",
			ClampMin = "1",
			ClampMax = "5",
			ToolTip = "Number of iterative domain warp passes (more = more distortion)"
		)
	)
	int32 WarpIterations = 2;

	/** Initial warp displacement strength */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "2.0",
			ClampMin = "0.0",
			ToolTip = "Initial warp displacement strength"
		)
	)
	float WarpDisplacement = 1.0f;

	/** Strength decay per warp iteration */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.0",
			ClampMin = "0.0",
			ToolTip = "Strength decay per warp iteration"
		)
	)
	float WarpFalloff = 0.5f;

	/** Output contrast adjustment (higher = more contrast) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.1",
			UIMax = "3.0",
			ClampMin = "0.1",
			ToolTip = "Output contrast adjustment (higher = more contrast)"
		)
	)
	float WarpContrast = 1.0f;

	/** Animation displacement magnitude */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "2.0",
			ClampMin = "0.0",
			ToolTip = "Animation displacement magnitude"
		)
	)
	float WarpAnimAmplitude = 0.5f;

	/** Animation speed multiplier */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "2.0",
			ClampMin = "0.0",
			ToolTip = "Animation speed multiplier"
		)
	)
	float WarpAnimationSpeed = 0.5f;

	/** X-axis warping noise scale */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.1",
			UIMax = "4.0",
			ClampMin = "0.1",
			ToolTip = "X-axis warping noise scale"
		)
	)
	float WarpXScale = 1.0f;

	/** Y-axis warping noise scale */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.1",
			UIMax = "4.0",
			ClampMin = "0.1",
			ToolTip = "Y-axis warping noise scale"
		)
	)
	float WarpYScale = 1.0f;

	/** Result noise sample scale */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.1",
			UIMax = "4.0",
			ClampMin = "0.1",
			ToolTip = "Result noise sample scale"
		)
	)
	float WarpNoiseScale = 1.0f;

	/** X-axis noise sampling offset */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			ToolTip = "X-axis noise sampling offset"
		)
	)
	FVector2D WarpXOffset = FVector2D(0.0f, 0.0f);

	/** Y-axis noise sampling offset */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			ToolTip = "Y-axis noise sampling offset"
		)
	)
	FVector2D WarpYOffset = FVector2D(100.0f, 0.0f);

	/** Result noise sampling offset */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			ToolTip = "Result noise sampling offset"
		)
	)
	FVector2D WarpNoiseOffset = FVector2D(0.0f, 100.0f);

	/** FBM octave count for warp noise */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "1",
			UIMax = "5",
			ClampMin = "1",
			ClampMax = "5",
			ToolTip = "FBM octave count for warp noise"
		)
	)
	int32 WarpOctaves = 3;

	/** How much finer detail each layer adds (higher = more dramatic detail contrast between layers) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && WarpOctaves > 1",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "3.0",
			ClampMin = "1.0",
			ToolTip = "How much finer detail each layer adds (higher = more dramatic detail contrast between layers)"
		)
	)
	float WarpLacunarity = 2.0f;

	/** How much influence each detail layer has (lower = smoother, higher = rougher) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && WarpOctaves > 1",
			EditConditionHides,
			UIMin = "0.3",
			UIMax = "1.0",
			ClampMin = "0.3",
			ToolTip = "How much influence each detail layer has (lower = smoother, higher = rougher)"
		)
	)
	float WarpGain = 0.5f;

	/** Initial FBM amplitude */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && WarpOctaves > 1",
			EditConditionHides,
			UIMin = "0.5",
			UIMax = "2.0",
			ClampMin = "0.5",
			ToolTip = "Initial FBM amplitude"
		)
	)
	float WarpInitialAmplitude = 1.0f;

	/** Final flow strength multiplier for warp field */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.0",
			ClampMin = "0.0",
			ToolTip = "Final flow strength multiplier for warp field"
		)
	)
	float WarpFlowStrength = 0.5f;

	/** Distance at which warp attenuation begins (full strength inside) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which warp attenuation begins (full strength inside)"
		)
	)
	float WarpAttenuationStart = 0.0f;

	/** Distance at which warp influence reaches zero */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "0.0",
			UIMax = "1.5",
			ClampMin = "0.0",
			ToolTip = "Distance at which warp influence reaches zero"
		)
	)
	float WarpAttenuationEnd = 0.5f;

	/** How sharply the warp effect fades with distance (higher = more abrupt fade) */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "AuroraControlPoint",
		meta = (
			EditCondition = "Type == EControlPointType::Warp && Range == EControlPointRange::Local",
			EditConditionHides,
			UIMin = "1.0",
			UIMax = "8.0",
			ClampMin = "0.001",
			ToolTip = "How sharply the warp effect fades with distance (higher = more abrupt fade)"
		)
	)
	float WarpExponent = 2.0f;
};
