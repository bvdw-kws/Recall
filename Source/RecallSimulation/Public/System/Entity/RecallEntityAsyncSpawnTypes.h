// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallEntityAsyncSpawnTypes.generated.h"

struct FMassEntityManager;

/**
 * Custom parameters that can be passed over along a spawn request and will be executed on spawn.
 */
USTRUCT()
struct RECALLSIMULATION_API FRecallEntityAsyncSpawnCommand
{
	GENERATED_BODY()

public:
	virtual ~FRecallEntityAsyncSpawnCommand() = default;

	/**
	 * Callback during entity creation, right after the entity was created.
	 * This is performed inside a FMassDeferredCreateCommand.
	 */
	virtual void OnSpawn(FMassEntityManager& System, const TArray<FMassEntityHandle>& Entities) const {}
};

USTRUCT()
struct RECALLSIMULATION_API FRecallEntityAsyncSpawnParameters
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int32 EntityCount = 1;
	
	UPROPERTY(VisibleAnywhere, meta=(BaseStruct="/Script/RecallSimulation.RecallEntityAsyncSpawnCommand", ExcludeBaseStruct))
	FInstancedStruct SpawnCommand;
	
	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle ParentEntity;
};

struct RECALLSIMULATION_API FRecallEntityAsyncSpawnContext
{
	FVector Position = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FRecallEntityAsyncSpawnParameters SpawnParameters;
};
