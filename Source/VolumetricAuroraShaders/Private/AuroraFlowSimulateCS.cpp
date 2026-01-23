/**
 * AuroraFlowSimulateCS.cpp
 * Shader registration for aurora flow simulation compute shader
 *
 * Registers FAuroraFlowSimulateCS shader class
 * Shader source: /VolumetricAuroraShaders/Private/AuroraFlowSimulateCS.usf
 * Entry point: MainCS
 */

#include "AuroraFlowSimulateCS.h"

IMPLEMENT_GLOBAL_SHADER(
	FAuroraFlowSimulateCS,
	"/VolumetricAuroraShaders/Private/AuroraFlowSimulateCS.usf",
	"MainCS",
	SF_Compute
);
