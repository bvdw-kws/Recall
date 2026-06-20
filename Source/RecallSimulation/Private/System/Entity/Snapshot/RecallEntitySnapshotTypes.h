// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassArchetypeTypes.h"
#include "System/Entity/RecallEntityTypes.h"

#include "RecallEntitySnapshotTypes.generated.h"

struct FMassEntityManager;

/*
* Snapshot of an entity
*/
USTRUCT()
struct FRecallEntitySnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle Entity;

	UPROPERTY(VisibleAnywhere)
	int32 AbsoluteIndex{ INDEX_NONE };

	UPROPERTY(VisibleAnywhere)
	TArray<FInstancedStruct> FragmentInstanceList;

	UPROPERTY(VisibleAnywhere)
	uint8 bMutable{ true };
};

USTRUCT()
struct FRecallSharedFragmentValuesSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<uint32> ConstSharedFragmentsHashes;

	UPROPERTY(VisibleAnywhere)
	TArray<uint32> SharedFragmentsHashes;

	void Set(const FMassEntityManager& EntityManager, const FMassArchetypeSharedFragmentValues& SharedFragmentValues);
	FMassArchetypeSharedFragmentValues Get(const FMassEntityManager& EntityManager) const;
};

USTRUCT()
struct FRecallChunkSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FRecallSharedFragmentValuesSnapshot SharedFragmentValues;

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallEntitySnapshot> EntitySnapshots;
};

USTRUCT()
struct FRecallArchetypeCompositionSnapshot
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere)
	TArray<const UScriptStruct*> FragmentTypes;

	UPROPERTY(VisibleAnywhere)
	TArray<const UScriptStruct*> TagTypes;

	UPROPERTY(VisibleAnywhere)
	TArray<const UScriptStruct*> ChunkFragmentTypes;

	UPROPERTY(VisibleAnywhere)
	TArray<const UScriptStruct*> SharedFragmentTypes;

	UPROPERTY(VisibleAnywhere)
	TArray<const UScriptStruct*> ConstSharedFragmentTypes;

public:
	void Set(const FMassArchetypeCompositionDescriptor& Composition);
	FMassArchetypeCompositionDescriptor Get() const;
};

/*
* Snapshot of our archetype
*/
USTRUCT()
struct FRecallArchetypeSnapshot
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	FRecallArchetypeCompositionSnapshot Composition;

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallChunkSnapshot> ChunkSnapshots;
};

/*
* Snapshot of our entity manager
*/
USTRUCT()
struct FRecallEntityManagerSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int32 SerialNumberGenerator = 0;

	UPROPERTY(VisibleAnywhere)
	int32 EntityCount = 0;

	UPROPERTY(VisibleAnywhere)
	FRecallEntityRegistry EntityRegistry;

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallArchetypeSnapshot> ArchetypeSnapshots;

	UPROPERTY(VisibleAnywhere)
	TMap<uint32, FInstancedStruct> SharedFragmentsMap;

	UPROPERTY(VisibleAnywhere)
	int32 LastEntityIndex{ INDEX_NONE };

public:
	void SetSharedFragmentsMap(const TMap<uint32, FSharedStruct>& InSharedFragmentsMap);
};
