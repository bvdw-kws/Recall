// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "RecallActorTypes.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallActorRegistryTypes.generated.h"

USTRUCT()
struct FRecallActorInstanceEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta=(BaseStruct="/Script/RecallSimulation.RecallActorRepresentationDesc", ExcludeBaseStruct))
	FInstancedStruct InstancedStruct;

	UPROPERTY(VisibleAnywhere)
	int32 Count = 0;
};

/*
* Our registry to keep track of actors between rollbacks.
*/
USTRUCT()
struct FRecallActorRegistry
{
	GENERATED_BODY()

	/** Our entries by handle. */
	UPROPERTY(VisibleAnywhere)
	TMap<FRecallActorHandle, uint32> HandleToInstanceHash;
	UPROPERTY(VisibleAnywhere)
	TMap<uint32, FRecallActorInstanceEntry> HashToEntryInstance;
};
