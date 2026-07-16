// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallEntitySnapshotSaveUtils.h"

#include "Async/ParallelFor.h"
#include "MassEntityManager.h"
#include "RecallEntitySnapshotCacheTypes.h"
#include "RecallEntitySnapshotTypes.h"

namespace Recall::EntitySnapshot::Save::Utils
{

void SaveEntityManager(const FMassEntityManager& EntityManager, int32 LastStaticEntitySerialNumber,
	FRecallEntityManagerSnapshot& NewSnapshot, TMap<FMassArchetypeHandle,
	FRecallArchetypeSaveCache>& ArchetypeCacheMap, TArray<FMassArchetypeHandle>& ArchetypeHandlesCache)
{
	const TArray<FMassEntityHandle> Entities = EntityManager.GetAllEntities();

	NewSnapshot.SerialNumberGenerator = EntityManager.GetSerialNumberGenerator();
	NewSnapshot.EntityCount = Entities.Num();

	// Cache our entities first so we know the size of the arrays, and we can order them by absolute index.
	for (const FMassEntityHandle& Entity : Entities)
	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Save_CacheArchetypes);
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Save CacheArchetypes")));

		const FMassArchetypeHandle ArchetypeHandle = EntityManager.GetArchetypeForEntity(Entity);

		FRecallArchetypeSaveCache& ArchetypeCache = ArchetypeCacheMap.FindOrAdd(ArchetypeHandle);
		if (ArchetypeCache.Entities.Num() == 0) // New archetype
		{
			ArchetypeHandlesCache.Add(ArchetypeHandle);
		}

		FRecallEntitySaveCache& EntityCache = ArchetypeCache.Entities.AddDefaulted_GetRef();
		EntityCache.Entity = Entity;
		EntityManager.GetArchetypeInternalIndexForEntity(Entity, ArchetypeHandle, EntityCache.AbsoluteIndex, EntityCache.ChunkIndex);

		NewSnapshot.LastEntityIndex = FMath::Max(Entity.Index, NewSnapshot.LastEntityIndex);
	}

	// Save our entities by using the cache as reference.
	NewSnapshot.ArchetypeSnapshots.SetNum(ArchetypeHandlesCache.Num());

	ParallelFor(ArchetypeHandlesCache.Num(),
		[&EntityManager, &ArchetypeHandlesCache, &ArchetypeCacheMap, &NewSnapshot, LastStaticEntitySerialNumber](int32 Index)
	{
		const FMassArchetypeHandle& ArchetypeHandle = ArchetypeHandlesCache[Index];
		FRecallArchetypeSaveCache& ArchetypeCache = ArchetypeCacheMap[ArchetypeHandle];
		FRecallArchetypeSnapshot& ArchetypeSnapshot = NewSnapshot.ArchetypeSnapshots[Index];

		SaveArchetypeSnapshot(EntityManager, LastStaticEntitySerialNumber, ArchetypeSnapshot, ArchetypeHandle, ArchetypeCache);
	});

	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Save_CopySharedFragments);
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Save CopySharedFragments")));

		NewSnapshot.SetSharedFragmentsMap(EntityManager.GetSharedFragmentValuesMap());
	}
}

void SaveArchetypeSnapshot(const FMassEntityManager& EntityManager, int32 LastStaticEntitySerialNumber,
	FRecallArchetypeSnapshot& ArcheTypeSnapshot,
	const FMassArchetypeHandle& ArchetypeHandle, FRecallArchetypeSaveCache& ArchetypeCache)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Save_CopyArchetypes);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Save CopyArchetype")));

	// Order our archetype entities by AbsoluteIndex
	ArchetypeCache.Entities.Sort([](const FRecallEntitySaveCache& lEntityCache, const FRecallEntitySaveCache& rEntityCache)
		{
			return lEntityCache.AbsoluteIndex < rEntityCache.AbsoluteIndex;
		}
	);

	const int32 ChunkCount = EntityManager.GetArchetypeChunkCount(ArchetypeHandle);

	ArcheTypeSnapshot.Composition.Set(EntityManager.GetArchetypeComposition(ArchetypeHandle));
	ArcheTypeSnapshot.ChunkSnapshots.SetNum(ChunkCount);

	struct FRecallMutableEntityCache
	{
		int32 ChunkIndex = INDEX_NONE;
		int32 EntityIndex = INDEX_NONE;
	};
	TArray<FRecallMutableEntityCache> MutableEntities;
	MutableEntities.Reserve(ArchetypeCache.Entities.Num());

	// Read all our archetype's entities but split them back into chunks
	int32 EntityIndex = 0;

	for (int32 ChunkIndex = 0; ChunkIndex < ChunkCount; ChunkIndex++)
	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Save_CopyChunk);
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Save CopyChunk")));

		FRecallChunkSnapshot& ChunkSnapshot = ArcheTypeSnapshot.ChunkSnapshots[ChunkIndex];
		ChunkSnapshot.SharedFragmentValues.Set(EntityManager, EntityManager.GetArchetypeSharedFragmentValues(ArchetypeHandle, ChunkIndex));

		int32 ChunkEntityCount = 0;
		for (int32 ChunkEntityCounterIdx = EntityIndex; ChunkEntityCounterIdx < ArchetypeCache.Entities.Num(); ChunkEntityCounterIdx++)
		{
			const FRecallEntitySaveCache& EntityCache = ArchetypeCache.Entities[ChunkEntityCounterIdx];
			if (EntityCache.ChunkIndex != ChunkIndex)
			{
				break;
			}
			ChunkEntityCount++;
		}

		ChunkSnapshot.EntitySnapshots.SetNum(ChunkEntityCount);

		for (int32 ChunkEntityIndex = 0; ChunkEntityIndex < ChunkEntityCount; EntityIndex++, ChunkEntityIndex++)
		{
			const FRecallEntitySaveCache& EntityCache = ArchetypeCache.Entities[EntityIndex];
			const FMassEntityHandle& Entity = EntityCache.Entity;

			FRecallEntitySnapshot& EntitySnapshot = ChunkSnapshot.EntitySnapshots[ChunkEntityIndex];
			EntitySnapshot.Entity = Entity;
			EntitySnapshot.AbsoluteIndex = EntityCache.AbsoluteIndex;
			EntitySnapshot.bMutable = Entity.SerialNumber > LastStaticEntitySerialNumber;

			if (EntitySnapshot.bMutable)
			{
				FRecallMutableEntityCache& MutableEntity = MutableEntities.AddDefaulted_GetRef();
				MutableEntity.ChunkIndex = ChunkIndex;
				MutableEntity.EntityIndex = ChunkEntityIndex;
			}
		}
	}

	TArray<FRecallChunkSnapshot>& ChunkSnapshots = ArcheTypeSnapshot.ChunkSnapshots;

	ParallelFor(MutableEntities.Num(), [&EntityManager, &MutableEntities, &ChunkSnapshots](int32 Index)
		{
			QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Save_GetEntityData);
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Save GetEntityData")));

			const FRecallMutableEntityCache& MutableEntity = MutableEntities[Index];

			FRecallEntitySnapshot& EntitySnapshot = ChunkSnapshots[MutableEntity.ChunkIndex].EntitySnapshots[MutableEntity.EntityIndex];
			EntityManager.GetEntityData(EntitySnapshot.Entity, EntitySnapshot.FragmentInstanceList);
		}
	);

#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
	checkf(EntityIndex == ArchetypeCache.Entities.Num(),
		TEXT("%hs We should have gone through all our entities"), __FUNCTION__);
#endif // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
}
	
} // namespace Recall::EntitySnapshot::Save::Utils
