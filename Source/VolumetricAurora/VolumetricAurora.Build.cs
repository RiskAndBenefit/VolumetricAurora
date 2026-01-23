// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VolumetricAurora : ModuleRules
{
    public VolumetricAurora(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "RenderCore",
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",   // added for using C++ Material Class
                "RHI",          // added for using C++ Material Class
                "Projects",
                "Renderer",
                "VolumetricAuroraShaders"

				// ... add private dependencies that you statically link with here ...	
			}
            );

        // added for using C++ Material Class
        // 런타임, 에디터 구별해서 모듈 정리 좀 해야함
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "MaterialEditor",
                    "AssetTools",
                    "AssetRegistry",  // VolumetricAurora.cpp(FindPresets)
		        }
            );
        }

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
