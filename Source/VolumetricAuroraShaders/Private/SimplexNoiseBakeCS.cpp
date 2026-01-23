#include "SimplexNoiseBakeCS.h"

IMPLEMENT_GLOBAL_SHADER(
	  FSimplexNoiseBakeCS,
	  "/VolumetricAuroraShaders/Private/SimplexNoiseBakeCS.usf",
	  "MainCS",
	  SF_Compute
);