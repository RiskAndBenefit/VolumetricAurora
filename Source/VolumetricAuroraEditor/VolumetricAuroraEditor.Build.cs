// Copyright (c) 2026 R&B. All rights reserved.

using UnrealBuildTool;

/// <summary>
/// Build configuration for VolumetricAuroraEditor module
/// Provides editor-only customizations for VolumetricAurora plugin
/// </summary>
public class VolumetricAuroraEditor : ModuleRules
{
	public VolumetricAuroraEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"VolumetricAurora",	// Access to runtime classes
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",				// Editor framework
				"EditorSubsystem",
				"PropertyEditor",		// IDetailCustomization Interface
				"EditorStyle",			// Editor UI styles
				"Projects",				// Plugin path utilities
				"RenderCore",			// Render targets
				"RHI",					// Rendering Hardware Interface
				"AssetTools",			// Asset creation utilities
				"AssetRegistry",
				"LevelEditor",			// Editor mode integration
				"InputCore",			// Input handling
				"UMG",					// UserWidget support for WBP integration
				"UMGEditor",			// WidgetBlueprint class for loading WBP assets
				"VolumetricAuroraShaders"
            }
		);
	}
}