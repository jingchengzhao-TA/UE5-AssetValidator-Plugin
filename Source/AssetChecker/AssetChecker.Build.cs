// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AssetChecker : ModuleRules
{
	public AssetChecker(ReadOnlyTargetRules Target) : base(Target)
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
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "AssetRegistry",    // 用于获取和扫描资产
                "ContentBrowser",   // 用于获取玩家当前在内容浏览器中选中的文件夹或资产
                "EditorStyle",      // 用于编辑器 UI 样式
                "MessageLog",       // 用于输出美观的报错日志
                "Blutility",        // 用于编辑器工具相关的蓝图支持
                "UMGEditor",        // 涉及部分UI
                "AssetTools",       // 用于处理资产相关的自动化工具
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
