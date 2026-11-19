// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

using UnrealBuildTool;

public class RecallTestSuite : ModuleRules
{
	public RecallTestSuite(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// The physics tests do a lot of Jolt work and must not leak JPH::uint64/int64 across translation units of a unity build.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"RecallOnline",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"JoltPhysics",
			"JoltPhysicsRuntime",
			"JoltPhysicsRuntimeCore",
			"MassEntity",
			"RecallPhysicsModule",
		});
	}
}
