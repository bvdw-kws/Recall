// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "System/Input/RecallInputQueueTypes.h"
#include "System/Player/RecallPlayerQueueTypes.h"
#include "System/Simulation/RecallSimulationControllerTypes.h"

#include "RecallReplayTypes.generated.h"

USTRUCT()
struct RECALLCORE_API FRecallWorldReplay
{
	GENERATED_BODY()
		
	UPROPERTY()
	int32 Seed{ 0 };

	UPROPERTY()
	FRecallPlayerQueue PlayerQueue;
};

USTRUCT()
struct RECALLCORE_API FRecallReplay
{
	GENERATED_BODY()

	UPROPERTY()
	FDateTime UtcTimeStamp;

	// Duration
	UPROPERTY()
	uint32 FrameCount {0};
	UPROPERTY()
	double Duration {0.0};
	
	UPROPERTY()
	FRecallSimulationStartParams StartParams;
	UPROPERTY()
	FString MapName;

	UPROPERTY()
	FRecallInputQueue InputQueue;

	UPROPERTY()
	TArray<FRecallWorldReplay> Worlds;

	UPROPERTY()
	TArray<uint8> DebugMenu;

	UPROPERTY()
	FInstancedStruct CustomData;
};
