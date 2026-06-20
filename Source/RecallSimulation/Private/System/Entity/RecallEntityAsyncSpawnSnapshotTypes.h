// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Asset/RecallAssetManagerTypes.h"
#include "System/Entity/RecallEntityAsyncSpawnTypes.h"

#include "RecallEntityAsyncSpawnSnapshotTypes.generated.h"

USTRUCT()
struct RECALLSIMULATION_API FRecallEntityAsyncSpawnRequest
{
	GENERATED_BODY()

	/**
	 * Keep track of the frame at which the entity spawn was requested.
	 */
	UPROPERTY(VisibleAnywhere)
	uint32 RequestFrame = 0;

	/**
	 * Path of the entity config to spawn.
	 */
	UPROPERTY(VisibleAnywhere, meta=(AllowedClasses="/Script/MassSpawner.MassEntityConfigAsset"))
	FSoftObjectPath EntityConfigAsset;

	/**
	 * Location and rotation of the entity to spawn.
	 */
	UPROPERTY(VisibleAnywhere)
	FVector Position = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere)
	FQuat Rotation = FQuat::Identity;

	/**
	 * Custom spawn parameters that can be accessed during entity creation.
	 */
	UPROPERTY(VisibleAnywhere)
	FRecallEntityAsyncSpawnParameters SpawnParameters;

	/**
	 * Handle to keep track of the entity config asset.
	 */
	UPROPERTY(VisibleAnywhere)
	FRecallAssetLoadHandle EntityConfigAssetHandle;
};

USTRUCT()
struct RECALLSIMULATION_API FRecallEntityAsyncSpawnQueue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallEntityAsyncSpawnRequest> Requests;

	FORCEINLINE void Reset()
	{
		Requests.Reset();
	}
};
