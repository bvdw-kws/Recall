// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

using System;
using System.IO;
using System.Linq;
using UnrealBuildTool;

public class RecallSimulation : ModuleRules
{
	public RecallSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "VariableCollectionModule" });
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"MassCore",
			"MassEntity",
			"MassSimulation",
			"MassSpawner",
			"RecallCore",
			"RecallSignals",
			"NetCore",
			"StateTreeModule",
			"Niagara",
			"LevelSequence",
			"MovieScene",
			"GameplayTags",
			"LevelSequence",
		});

		bool bWithMultiWorld = Target.EnablePlugins.Contains("MultiWorld");
		if (bWithMultiWorld)
		{
			PrivateDependencyModuleNames.Add("MultiWorld");
		}
		
		// Uncomment if you are debugging online de-sync
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PublicDefinitions.Add("RECALL_DESYNC_ENABLED");
		}
	}
}