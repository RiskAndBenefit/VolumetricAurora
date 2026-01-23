#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Windows/AllowWindowsPlatformTypes.h"

class VOLUMETRICAURORASHADERS_API FSimplexNoiseBakeCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FSimplexNoiseBakeCS)
	SHADER_USE_PARAMETER_STRUCT(FSimplexNoiseBakeCS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// 출력 텍스처
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutTexture)
		// Noise 파라미터
		SHADER_PARAMETER(int32, NoiseType)
		SHADER_PARAMETER(float, NoiseScale)
		SHADER_PARAMETER(int32, Octaves)
		SHADER_PARAMETER(float, Persistence)
		SHADER_PARAMETER(float, Lacunarity)
		SHADER_PARAMETER(FVector2f, Offset)
		SHADER_PARAMETER(float, Time)		// 애니메이션용
		SHADER_PARAMETER(float, Seed)		// 노이즈 패턴 시드
		SHADER_PARAMETER(float, Amplitude)	// 초기 진폭
		SHADER_PARAMETER(float, Contrast)	// 대비 조정 (pow 지수)
		// Curl Noise 전용
		SHADER_PARAMETER(float, BaseFrequency)	// Curl 기본 주파수
		SHADER_PARAMETER(FVector2f, AnimDir)	// Curl 애니메이션 방향
		SHADER_PARAMETER(int32, IsDual)		// 듀얼 샘플링 여부
		SHADER_PARAMETER(int32, IsSeamless)	// Seamless tiling 여부
	END_SHADER_PARAMETER_STRUCT()
};
