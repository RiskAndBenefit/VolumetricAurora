#include "VolumetricAuroraShaders.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

void FVolumetricAuroraShadersModule::StartupModule()
{
	
	TArray<FString> SearchPaths;
	SearchPaths.Add(FPaths::ProjectPluginsDir() / TEXT("VolumetricAurora/Shaders"));
	SearchPaths.Add(FPaths::EnginePluginsDir() / TEXT("VolumetricAurora/Shaders"));
	SearchPaths.Add(FPaths::EnterprisePluginsDir() / TEXT("VolumetricAurora/Shaders")); 

	FString PluginShaderDir;

	for (const FString& Path : SearchPaths)
	{
		if (FPaths::DirectoryExists(Path))
		{
			PluginShaderDir = Path;
			break;
		}
	}

	if (PluginShaderDir.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("VolumetricAurora/Shader folder not found in Project, Engine, EnterprisePluginsDir"));
	}
	
	AddShaderSourceDirectoryMapping(TEXT("/VolumetricAuroraShaders"), PluginShaderDir);
}

void FVolumetricAuroraShadersModule::ShutdownModule() {}

IMPLEMENT_MODULE(FVolumetricAuroraShadersModule, VolumetricAuroraShaders)