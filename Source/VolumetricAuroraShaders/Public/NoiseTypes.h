// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NoiseTypes.generated.h"

UENUM(BlueprintType)
enum class EVANoiseType : uint8
{
	Value		UMETA(DisplayName = "Value Noise"),
	Gradient	UMETA(DisplayName = "Gradient Noise"),
	Perlin		UMETA(DisplayName = "Perlin Noise"),
	Simplex		UMETA(DisplayName = "Simplex Noise"),
	Curl		UMETA(DisplayName = "Curl Noise")
};