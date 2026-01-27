// Copyright (c) 2026 R&B. All rights reserved.

/**
 * AuroraFlowSimulateCS.h
 * Aurora flow simulation compute shader declarations
 *
 * Implements Semi-Lagrangian advection-based particle simulation
 * with multiple control point types and obstacle interaction.
 *
 * Features:
 * - Frame-rate independent density simulation
 * - Control points: gravity, vortex, spiral, curl noise
 * - Obstacle deflection and particle extinction
 * - Aurora element-based emission/extinction control
 */

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

struct FControlPointInfoGPU
{
	// 16 bytes
	uint32		Type;				// Control point type (e.g. CP_GRAVITY, CP_VORTEX, CP_CURL, etc.)
	uint32		IsGlobal;			// Whether this control point applies globally (ignores attenuation start/end)
	FVector2f	Position;			// Control point center position in UV space
};

struct FSingleForceControlPointGPU
{
	// 16 bytes
	float	ForceStrength;			// Primary force magnitude (signed if direction is implicit)
	float	AttenuationStart;		// Distance at which attenuation begins (full strength inside)
	float	AttenuationEnd;			// Distance at which force influence reaches zero
	float	AttenuationExponent;	// Exponent shaping edge falloff (higher = sharper drop near boundary)
};

struct FDoubleForceControlPointGPU
{
	// 16 bytes
	float	ForceStrength;
	float	AttenuationStart;
	float	AttenuationEnd;
	float	AttenuationExponent;

	// 16 bytes
	// Secondary force channel (used by control points with multiple force components,
	// e.g. Source+Emission, Sink+Fade, Radial+Rotation)
	float	ForceStrength2;
	float	AttenuationStart2;
	float	AttenuationEnd2;
	float	AttenuationExponent2;
};

struct FTripleForceControlPointGPU
{
	// 16 bytes
	float	ForceStrength;
	float	AttenuationStart;
	float	AttenuationEnd;
	float	AttenuationExponent;

	// 16 bytes
	// Secondary force channel (used by control points with multiple force components,
	// e.g. Source+Emission, Sink+Fade, Radial+Rotation)
	float	ForceStrength2;
	float	AttenuationStart2;
	float	AttenuationEnd2;
	float	AttenuationExponent2;

	// 16 bytes
	// Third force channel
	// e.g. Spiral(Vortex + Sink + Attenuation)
	float	ForceStrength3;
	float	AttenuationStart3;
	float	AttenuationEnd3;
	float	AttenuationExponent3;
};

struct FDipoleControlPointGPU
{
	// 16 bytes
	float	ForceStrength;
	float	AttenuationStart;
	float	AttenuationEnd;
	float	AttenuationExponent;

	// 16 bytes
	FVector2f	DipoleDirection;	// Dipole axis direction (normalized)
	float		_padding1[2];
};

struct FCurlControlPointGPU
{
	// 16 bytes
	float	ForceStrength;
	float	AttenuationStart;
	float	AttenuationEnd;
	float	AttenuationExponent;

	// 16 bytes
	/* For curl only */
	float	CurlFrequency;		// Curl noise base frequency
	float	CurlAnimationSpeed;	// Curl noise temporal animation speed
	uint32	CurlOctaves;		// Number of curl noise octaves
	float	CurlLacunarity;		// Frequency multiplier per octave

	// 16 bytes
	float	CurlGain;			// Amplitude multiplier per octave
	float	CurlAmplitude;		// Base amplitude of curl noise
	float	_padding[2];
};

struct FWarpControlPointGPU
{
	// Block 1: 16 bytes - Attenuation
	float	FlowStrength;			// Output flow strength multiplier
	float	AttenuationStart;
	float	AttenuationEnd;
	float	AttenuationExponent;

	// Block 2: 16 bytes - Warp control
	uint32	Iterations;				// Number of iterative warp passes
	float	Displacement;			// Initial warp displacement strength
	float	Falloff;				// Strength decay per iteration
	float	Contrast;				// Output contrast adjustment

	// Block 3: 16 bytes - Animation and scales
	float	AnimAmplitude;			// Animation displacement magnitude
	float	AnimationSpeed;			// Animation speed multiplier
	float	XScale;					// X-axis warping noise scale
	float	YScale;					// Y-axis warping noise scale

	// Block 4: 16 bytes - Result scale and FBM
	float	NoiseScale;				// Result noise sample scale
	uint32	Octaves;				// FBM octave count
	float	Lacunarity;				// Frequency multiplier per octave
	float	Gain;					// Amplitude multiplier per octave

	// Block 5: 16 bytes - FBM amplitude, XOffset
	float	InitialAmplitude;		// Initial FBM amplitude
	FVector2f	XOffset;			// X-axis noise sampling offset
	float	_padding1;

	// Block 6: 16 bytes - YOffset, NoiseOffset
	FVector2f	YOffset;			// Y-axis noise sampling offset
	FVector2f	NoiseOffset;		// Result noise sampling offset
};

/**
 * Aurora flow simulation compute shader
 * Simulates density field evolution using Semi-Lagrangian advection
 * Entry point: MainCS in AuroraFlowSimulateCS.usf
 * Thread group: 8x8x1
 */
class VOLUMETRICAURORASHADERS_API FAuroraFlowSimulateCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FAuroraFlowSimulateCS)
	SHADER_USE_PARAMETER_STRUCT(FAuroraFlowSimulateCS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Double-buffered density textures
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, FrontBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, BackBuffer)
		SHADER_PARAMETER_SAMPLER(SamplerState, Sampler)

		// Simulation parameters
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(FVector2f, BaseFlow)

		// Control point type info
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FControlPointInfoGPU>, ControlPointsInfo)
		SHADER_PARAMETER(uint32, NumControlPoints)
	
		// Single force control points
		SHADER_PARAMETER_RDG_BUFFER_SRV(
			StructuredBuffer<FSingleForceControlPointGPU>,
			SingleForceControlPoints
		)

		// Double force control points
		SHADER_PARAMETER_RDG_BUFFER_SRV(
			StructuredBuffer<FDoubleForceControlPointGPU>,
			DoubleForceControlPoints
		)

		// Triple force control points
		SHADER_PARAMETER_RDG_BUFFER_SRV(
			StructuredBuffer<FTripleForceControlPointGPU>,
			TripleForceControlPoints
		)

		// Control points with distinct parameters
		SHADER_PARAMETER_RDG_BUFFER_SRV(
			StructuredBuffer<FDipoleControlPointGPU>,
			DipoleControlPoints
		)

		SHADER_PARAMETER_RDG_BUFFER_SRV(
			StructuredBuffer<FCurlControlPointGPU>,
			CurlControlPoints
		)

		SHADER_PARAMETER_RDG_BUFFER_SRV(
			StructuredBuffer<FWarpControlPointGPU>,
			WarpControlPoints
		)

		// Obstacle system
		SHADER_PARAMETER_TEXTURE(Texture2D, ObstacleMap)
		SHADER_PARAMETER_SAMPLER(SamplerState, ObstacleSampler)
		SHADER_PARAMETER(uint32, bHasObstacle)

		// Aurora element control
		SHADER_PARAMETER_TEXTURE(Texture2D, AuroraElementsMap)
		SHADER_PARAMETER_SAMPLER(SamplerState, AuroraElementsSampler)

		// Emitter noise parameters
		SHADER_PARAMETER(float, EmitterNoiseFrequency)
		SHADER_PARAMETER(float, EmitterNoiseSpeed)
		SHADER_PARAMETER(float, EmitterNoiseStrength)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(
		const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(
			Parameters.Platform,
			ERHIFeatureLevel::SM5
		);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
	}
};
