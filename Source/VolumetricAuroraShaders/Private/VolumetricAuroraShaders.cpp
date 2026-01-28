// Copyright (c) 2026 R&B. All rights reserved.

#include "VolumetricAuroraShaders.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ShaderCore.h"

void FVolumetricAuroraShadersModule::StartupModule()
{
#if WITH_EDITOR
	FString PluginShaderDir;
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"));

	if (Plugin.IsValid())
	{
		FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		if (FPaths::DirectoryExists(ShaderDir))
		{
			PluginShaderDir = ShaderDir;
		}
	}

	if (PluginShaderDir.IsEmpty())
	{
		FString RelativeShaderPath = TEXT("VolumetricAurora/Shaders");

		TArray<FString> SearchPaths;
		SearchPaths.Add(FPaths::ProjectPluginsDir());
		SearchPaths.Add(FPaths::EnginePluginsDir());
		SearchPaths.Add(FPaths::EnterprisePluginsDir());

		for (const FString& SearchPath : SearchPaths)
		{
			FString Path = SearchPath / RelativeShaderPath;

			if (FPaths::DirectoryExists(Path))
			{
				PluginShaderDir = Path;
				break;
			}
			else
			{
				Path = SearchPath / TEXT("Marketplace") / RelativeShaderPath;

				if (FPaths::DirectoryExists(Path))
				{
					PluginShaderDir = Path;
					break;
				}
			}
		}
	}

	FPaths::CollapseRelativeDirectories(PluginShaderDir);

	if (!PluginShaderDir.IsEmpty())
	{
		AddShaderSourceDirectoryMapping(TEXT("/VolumetricAuroraShaders"), PluginShaderDir);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VolumetricAurora/Shaders folder does not exist"));
	}
#endif
}

void FVolumetricAuroraShadersModule::ShutdownModule() {}

IMPLEMENT_MODULE(FVolumetricAuroraShadersModule, VolumetricAuroraShaders)