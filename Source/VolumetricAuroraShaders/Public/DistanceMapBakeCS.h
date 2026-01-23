/**
 * DistanceMapBakeCS.h
 * Jump Flooding Algorithm (JFA) compute shader declarations
 *
 * Provides three-pass distance field generation from binary obstacle masks:
 * 1. Initialize: Convert mask to seed coordinates
 * 2. Jump Flooding: Propagate nearest obstacle information
 * 3. Finalize: Calculate signed distance field with normals
 *
 * Output: RGBA texture (RG=Normal, B=Distance, A=Mask)
 */

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

// ============================================================================
// Pass 1: Initialize - Convert mask to seed coordinates
// ============================================================================

/**
 * Initialize shader for JFA distance field generation
 * Converts binary obstacle mask to seed coordinate buffer
 * Entry point: InitializeCS in DistanceMapBakeCS.usf
 */
class VOLUMETRICAURORASHADERS_API FDistanceMapInitCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDistanceMapInitCS)
	SHADER_USE_PARAMETER_STRUCT(FDistanceMapInitCS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input: Grayscale mask texture (white=obstacle, black=empty)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InputMask)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		// Output: Seed coordinates (RG = closest obstacle pixel, BA = unused)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SeedBuffer)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
	}
};

// ============================================================================
// Pass 2: Jump Flooding - Propagate nearest seed information
// ============================================================================

/**
 * Jump Flooding Algorithm shader pass
 * Propagates nearest obstacle coordinates across the field
 * Requires log2(TextureSize) passes with decreasing JumpStep
 * Entry point: JumpFloodingCS in DistanceMapBakeCS.usf
 */
class VOLUMETRICAURORASHADERS_API FDistanceMapJFACS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDistanceMapJFACS)
	SHADER_USE_PARAMETER_STRUCT(FDistanceMapJFACS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input: Previous pass seed buffer
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InputSeeds)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		// Output: Updated seed buffer
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputSeeds)
		// Jump step size (halves each pass: N/2, N/4, N/8, ..., 1)
		SHADER_PARAMETER(int32, JumpStep)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
	}
};

// ============================================================================
// Pass 3: Finalize - Calculate distance and normals, encode to RGBA
// ============================================================================

/**
 * Finalize shader for JFA distance field generation
 * Computes final distance values and surface normals from JFA result
 * Encodes output as RGBA: (Normal.xy, Distance, Mask)
 * Entry point: FinalizeCS in DistanceMapBakeCS.usf
 */
class VOLUMETRICAURORASHADERS_API FDistanceMapFinalizeCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FDistanceMapFinalizeCS)
	SHADER_USE_PARAMETER_STRUCT(FDistanceMapFinalizeCS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input: Final seed buffer from JFA
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, SeedBufferFinal)
		SHADER_PARAMETER_SAMPLER(SamplerState, SeedSampler)
		// Input: Original mask (for normal calculation)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, MaskInput)
		SHADER_PARAMETER_SAMPLER(SamplerState, MaskSampler)
		// Output: RGBA texture (RG=Normal, B=Distance, A=Mask)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
	}
};
