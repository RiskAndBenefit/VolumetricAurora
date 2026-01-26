// Copyright (c) 2026 R&B. All rights reserved.

#include "SimplexNoiseBakeCS.h"

IMPLEMENT_GLOBAL_SHADER(
	  FSimplexNoiseBakeCS,
	  "/VolumetricAuroraShaders/Private/SimplexNoiseBakeCS.usf",
	  "MainCS",
	  SF_Compute
);