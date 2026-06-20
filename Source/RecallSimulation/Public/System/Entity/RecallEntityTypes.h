// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityHandle.h"
#include "System/Player/RecallPlayerQueueTypes.h"

#include "RecallEntityTypes.generated.h"

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallEntity, Log, All);

class UMassEntityConfigAsset;

/*
* Data to copy our dynamic entities.
*/
USTRUCT()
struct RECALLSIMULATION_API FRecallControllerEntityData
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	TObjectPtr<const UMassEntityConfigAsset> EntityConfig;

	UPROPERTY(Transient, VisibleAnywhere)
	TArray<FInstancedStruct> FragmentInstanceList;
};

USTRUCT()
struct RECALLSIMULATION_API FRecallControllerEntityCreationContext
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	FString OwnerControllerId;

	UPROPERTY(Transient, VisibleAnywhere)
	TObjectPtr<const UMassEntityConfigAsset> EntityConfig;
	
	UPROPERTY(Transient, VisibleAnywhere)
	FRecallPlayerSpawnParameters SpawnParameters;
};

/*
* Parameters passed when spawning our entity.
*/
USTRUCT()
struct RECALLSIMULATION_API FRecallControllerEntitySpawnParameters
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	FString OwnerControllerId;
	
	UPROPERTY(Transient, VisibleAnywhere)
	FRecallPlayerSpawnParameters SpawnParameters;
};

/*
* Player entity entry in the entity registry.
*/
USTRUCT()
struct FRecallControllerEntityEntry
{
	GENERATED_BODY()

	/* ID of the player owning this entry. */
	UPROPERTY(VisibleAnywhere)
	FString ControllerID;

	/* Entity that this player owns. */
	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle EntityHandle;

	/* Entity config used by this player. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<const UMassEntityConfigAsset> EntityConfig;
};

/*
* The register of all our entities
*/
USTRUCT()
struct FRecallEntityRegistry
{
	GENERATED_BODY()

	// Keep track of our entity for each player
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallControllerEntityEntry> ControllerEntries;

	UPROPERTY(VisibleAnywhere, meta=(AllowedClasses="/Script/MassSpawner.MassEntityConfigAsset"))
	TArray<FSoftObjectPath> BuiltEntityConfigs;

};
