// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Types/AuroraTypes.h"

/**
 *
 */
class VOLUMETRICAURORASHADERS_API FAuroraSDFBakeCS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FAuroraSDFBakeCS)
	
	SHADER_USE_PARAMETER_STRUCT(FAuroraSDFBakeCS, FGlobalShader)
		 
		
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVector4f>, InPoints)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FAuroraSplineInfo>, InSplines)
		SHADER_PARAMETER(FVector2f, InMapCenter)
		SHADER_PARAMETER(int32, InSplineCount)
		SHADER_PARAMETER(float, InMapSize)
		
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutSDF)
	END_SHADER_PARAMETER_STRUCT()

	
};
