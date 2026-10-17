// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DataBridge : ModuleRules
{
	public DataBridge(ReadOnlyTargetRules Target) : base(Target)
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
				"CoreUObject",
				"Engine",
				"Json",
            }
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "UnrealEd",       // FScopedTransaction, 에디터 유틸
				"AssetTools",     // 새 에셋 생성
				"AssetRegistry",  // 경로로 에셋 찾기, 참조 카탈로그
				"GameplayTags",   // 범용 태그 포매터 (엔진 모듈이라 독립성 문제 없음)
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
