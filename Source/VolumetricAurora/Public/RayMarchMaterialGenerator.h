// RayMarchMaterialGenerator.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RayMarchMaterialGenerator.generated.h"

/*
 * Utility Calss that creating Ray Marching Material with C++
 */

UCLASS(Blueprintable, BlueprintType)
class VOLUMETRICAURORA_API URayMarchMaterialGenerator : public UObject
{
	GENERATED_BODY()

public:
	/*
	 * Create Ray Marching Material
	 * Callable at Editor
	 */
	UFUNCTION(BlueprintCallable, Category = "RayMarch|Material")
	static UMaterial* GenerateRayMarchMaterial();

private:
	/* Custom Node에 들어갈 HLSL 코드 생성 */
	static FString GetRayMarchHLSLCode();
};