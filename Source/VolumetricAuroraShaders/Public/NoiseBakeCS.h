// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/**
 *
 */
class VOLUMETRICAURORASHADERS_API FNoiseBakeCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FNoiseBakeCS);
	SHADER_USE_PARAMETER_STRUCT(FNoiseBakeCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RenderTarget)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InputNoiseTex)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputNoiseSampler)
		SHADER_PARAMETER(float, SwirlStrength)
		SHADER_PARAMETER(float, SwirlRadius)
		SHADER_PARAMETER(FVector2f, SwirlCenter)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 32);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
	}
};