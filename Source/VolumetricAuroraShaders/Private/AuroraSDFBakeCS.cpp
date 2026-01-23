// Fill out your copyright notice in the Description page of Project Settings.


#include "AuroraSDFBakeCS.h"

IMPLEMENT_GLOBAL_SHADER(FAuroraSDFBakeCS, "/VolumetricAuroraShaders/Private/AuroraSDFBakeCS.usf", "MainCS", SF_Compute);