// Copyright (c) 2026 R&B. All rights reserved.

using UnrealBuildTool;

public class VolumetricAuroraShaders : ModuleRules
{
    public VolumetricAuroraShaders(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "Projects" 
        });
    }
}