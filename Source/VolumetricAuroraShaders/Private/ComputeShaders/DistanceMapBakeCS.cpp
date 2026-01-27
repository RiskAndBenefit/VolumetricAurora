// Copyright (c) 2026 R&B. All rights reserved.

/**
 * DistanceMapBakeCS.cpp
 * Shader registration for Jump Flooding Algorithm compute shaders
 *
 * Registers three shader passes:
 * - FDistanceMapInitCS: Initialize seed buffer from obstacle mask
 * - FDistanceMapJFACS: Jump Flooding propagation
 * - FDistanceMapFinalizeCS: Calculate final distance field with normals
 *
 * Shader source: /VolumetricAuroraShaders/Private/DistanceMapBakeCS.usf
 */

#include "ComputeShaders/DistanceMapBakeCS.h"

// Initialize pass shader
IMPLEMENT_GLOBAL_SHADER(
	FDistanceMapInitCS,
	"/VolumetricAuroraShaders/Private/DistanceMapBakeCS.usf",
	"InitializeCS",
	SF_Compute
);

// Jump Flooding pass shader
IMPLEMENT_GLOBAL_SHADER(
	FDistanceMapJFACS,
	"/VolumetricAuroraShaders/Private/DistanceMapBakeCS.usf",
	"JumpFloodingCS",
	SF_Compute
);

// Finalize pass shader
IMPLEMENT_GLOBAL_SHADER(
	FDistanceMapFinalizeCS,
	"/VolumetricAuroraShaders/Private/DistanceMapBakeCS.usf",
	"FinalizeCS",
	SF_Compute
);
