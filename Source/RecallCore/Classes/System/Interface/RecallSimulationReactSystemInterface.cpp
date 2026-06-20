// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSimulationReactSystemInterface.h"

#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"

URecallSimulationReactSystemInterface::URecallSimulationReactSystemInterface(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> URecallSimulationReactSystemInterface::GetSimulationReactSystems(UWorld* World)
{
	TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications;

	for (UWorldSubsystem* WorldSystem : World->GetSubsystemArrayCopy<UWorldSubsystem>())
	{
		if (WorldSystem->GetClass()->ImplementsInterface(URecallSimulationReactSystemInterface::StaticClass()))
		{
			SimNotifications.Add(TScriptInterface<IRecallSimulationReactSystemInterface>(WorldSystem));
		}
	}

	return SimNotifications;
}
