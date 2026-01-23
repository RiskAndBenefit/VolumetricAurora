// Fill out your copyright notice in the Description page of Project Settings.


#include "NoiseBakeCS.h"


IMPLEMENT_GLOBAL_SHADER(FNoiseBakeCS, "/VolumetricAuroraShaders/Private/SwirlDistortion.usf", "MainCS", SF_Compute);