// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

using System;
using System.IO;
using System.Linq;
using UnrealBuildTool;

public class RecallOnline : ModuleRules
{
	public RecallOnline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "RecallCore", "EasyOnline", "ModularGameplay" });
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"MassEntity",
            "MassSpawner",
			"RecallFrontend",
			"RecallSimulation",
			"RecallSnapshot",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"CommonGame",
			"AIModule",
			"DeveloperSettings",
			"CommonUI",
			"ExtendedCommonUI",
			"EasyDataTransferModule",
		});

		bool bWithMultiWorld = Target.EnablePlugins.Contains("MultiWorld");
		if (bWithMultiWorld)
		{
			PrivateDependencyModuleNames.Add("MultiWorld");
		}

		if (Target.EnablePlugins.Contains("DebugMenu"))
		{
			PrivateDependencyModuleNames.Add("DebugMenu");
		}

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG" });
	}
}