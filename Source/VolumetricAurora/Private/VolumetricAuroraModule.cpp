// Copyright (c) 2026 R&B. All rights reserved.

#include "VolumetricAuroraModule.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FVolumetricAuroraModule"

void FVolumetricAuroraModule::StartupModule()
{
	FString PluginBaseDir;

	// 1. Try IPluginManager (Standard)
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"));
	if (Plugin.IsValid())
	{
		PluginBaseDir = Plugin->GetBaseDir();
	}

	// 2. Try Project Plugins Directory (Common for project plugins)
	if (PluginBaseDir.IsEmpty())
	{
		FString Candidate = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("VolumetricAurora"));
		if (FPaths::DirectoryExists(Candidate))
		{
			PluginBaseDir = Candidate;
		}
	}

	// 3. Try Module Filename (Fallback for early loading)
	if (PluginBaseDir.IsEmpty())
	{
		// 상단에 #include "Interfaces/IPluginManager.h" 추가 필수
		FString ModulePath = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"))->GetBaseDir();

		if (!ModulePath.IsEmpty())
		{
			// Path: .../Plugins/VolumetricAurora/Binaries/Platform/Module.dll
			PluginBaseDir = FPaths::GetPath(FPaths::GetPath(FPaths::GetPath(ModulePath)));
		}
	}

	// 4. Manual reconstruction from Engine/Project root (Last resort)
	if (PluginBaseDir.IsEmpty() || !FPaths::DirectoryExists(PluginBaseDir))
	{
		// Try to find it relative to the project file
		FString ProjectDir = FPaths::ProjectDir();
		if (!ProjectDir.IsEmpty())
		{
			FString Candidate = FPaths::Combine(ProjectDir, TEXT("Plugins/VolumetricAurora"));
			if (FPaths::DirectoryExists(Candidate))
			{
				PluginBaseDir = Candidate;
			}
		}
	}

	if (!PluginBaseDir.IsEmpty())
	{
		FString PluginShaderDir = FPaths::Combine(PluginBaseDir, TEXT("Shaders"));
		PluginShaderDir = FPaths::ConvertRelativePathToFull(PluginShaderDir);

		if (FPaths::DirectoryExists(PluginShaderDir))
		{
			AddShaderSourceDirectoryMapping(TEXT("/VolumetricAurora"), PluginShaderDir);
			UE_LOG(LogTemp, Log, TEXT("VolumetricAurora: Successfully mapped shader directory: %s"), *PluginShaderDir);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("VolumetricAurora: Found plugin base but Shaders directory is missing at: %s"), *PluginShaderDir);
		}
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("VolumetricAurora: Critical Error - Could not resolve plugin directory during PostConfigInit! Shader mapping failed."));
	}
}

void FVolumetricAuroraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVolumetricAuroraModule, VolumetricAurora)