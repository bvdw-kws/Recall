// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Simulation/RecallSimulationUtils.h"

#include "Engine/World.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Simulation/RecallSimulationSubsystem.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#include "Utility/Slowmo/RecallSlowmoUtils.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

namespace Recall::Simulation::Utils
{
	
uint32 GetFrame(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		if (const URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(WorldContextObject->GetWorld()))
		{
			return SimulationSystem->GetSimulationFrame();
		}
	}
	return 0;
}

double GetTimeSeconds(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		if (const URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(WorldContextObject->GetWorld()))
		{
			return SimulationSystem->GetSimulationTime();
		}
	}
	return 0;
}

int32 GetFramesPerSeconds(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		if (const URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(WorldContextObject->GetWorld()))
		{
			return SimulationSystem->GetFramesPerSeconds();
		}
	}
	return 60;
}

float GetFixedDeltaTime(const UObject* WorldContextObject)
{
	const int32 FramesPerSeconds = GetFramesPerSeconds(WorldContextObject);

	if (FramesPerSeconds > 0)
	{
		return 1.0f / static_cast<float>(FramesPerSeconds);
	}
	else
	{
		return 0.0f;
	}
}

float GetDilatedFixedDeltaTime(const UObject* WorldContextObject)
{
	const float FixedDeltaTime = GetFixedDeltaTime(WorldContextObject);
	const float TimeDilatation = Recall::Slowmo::Utils::GetTimeDilatation(WorldContextObject);
	return FixedDeltaTime * TimeDilatation;
}

float GetRepresentationDeltaFrame(const UObject* WorldContextObject)
{
	checkf(IsSimulationRenderPhase(WorldContextObject),
		TEXT("%hs This is meant for representation only"), __FUNCTION__);
	
	if (WorldContextObject)
	{
		if (const URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(WorldContextObject->GetWorld()))
		{
			return SimulationSystem->GetSimulationRenderDeltaFrame();
		}
	}
	return 1.0f;
}
	
float GetRepresentationSpeedScale(const UObject* WorldContextObject)
{
	checkf(IsSimulationRenderPhase(WorldContextObject),
		TEXT("%hs This is meant for representation only"), __FUNCTION__);
	
	if (WorldContextObject)
	{
		if (const URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(WorldContextObject->GetWorld()))
		{
			return SimulationSystem->GetSimulationRenderSpeedScale();
		}
	}
	return 1.0f;
}

bool HasSimulationStarted(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);

	if (const URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		return MultiSimSystem->HasSimulationStarted();
	}
	return false;
}

bool IsSimulationProcessingPhase(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);

	if (const URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		return MultiSimSystem->IsSimulationProcessingPhase();
	}

	return false;
}

bool IsSimulationRenderPhase(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);

	if (const URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		return MultiSimSystem->IsRenderProcessingPhase();
	}

	return false;
}

void CheckSimulationProcessingPhase(const UObject* WorldContextObject, FString Reason /*= FString()*/)
{
	checkf(IsSimulationProcessingPhase(WorldContextObject), TEXT("Can not be executed outside of Simulation processing phase (Reason: %s)."), *Reason);
}

int32 GetMaxStepCount(const UObject* WorldContextObject)
{
	// Always allow enough steps for rollback and a minimum of 2 frames so restore can catch up.
	const int32 RollbackFrameCount = GetRollbackFrameCount(WorldContextObject);
	return FMath::Max(2, RollbackFrameCount + 1);
}

} // namespace Recall::Simulation::Utils
