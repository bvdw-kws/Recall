// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallEntitySnapshotRestoreUtils.h"

#include "Async/ParallelFor.h"
#include "MassArchetypeTypes.h"
#include "MassEntityManager.h"
#include "RecallEntitySnapshotCacheTypes.h"
#include "RecallEntitySnapshotTypes.h"

namespace Recall::EntitySnapshot::Restore::Utils
{

void RestoreEntityManager(FMassEntityManager& EntityManager, const FRecallEntityManagerSnapshot& Snapshot,
	int32 LastStaticEntitySerialNumber,	TMap<int32, FMassEntityHandle>& ReservedEntitiesCache,
	TMap<FMassArchetypeHandle, TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreMapCache,
	TArray<TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreCaches)
{
	const TArray<FRecallArchetypeSnapshot>& ArchetypeSnapshots = Snapshot.ArchetypeSnapshots;

	InitArchetypeRestoreCache(EntityManager, ArchetypeSnapshots, ArchetypeRestoreCaches, ArchetypeRestoreMapCache);

	PrepareMutableEntities(EntityManager, LastStaticEntitySerialNumber, Snapshot, ArchetypeRestoreMapCache);

	FCriticalSection EntityManagerLock;
	ParallelFor(ArchetypeSnapshots.Num(), [&EntityManager, &ArchetypeRestoreCaches, &EntityManagerLock](int32 Index)
	{
		FRecallArchetypeRestoreCache& ArchetypeRestoreCache = *ArchetypeRestoreCaches[Index];
		PrepareArchetypeRestoreCache(EntityManagerLock, EntityManager, ArchetypeRestoreCache);
	});

	// Reserve entities to be restored
	ReserveRestoreEntities(EntityManager, ArchetypeSnapshots, ArchetypeRestoreCaches,
		Snapshot.LastEntityIndex,ReservedEntitiesCache);

	// Restore entities from our archetype snapshot
	// It can be multithreaded because we already reserved our entities
	ParallelFor(Snapshot.ArchetypeSnapshots.Num(),
		[&EntityManagerLock, &EntityManager, &Snapshot, &ArchetypeRestoreCaches](int32 Index)
	{
		const FRecallArchetypeSnapshot& ArchetypeSnapshot = Snapshot.ArchetypeSnapshots[Index];
		FRecallArchetypeRestoreCache& ArchetypeRestoreCache = *ArchetypeRestoreCaches[Index];
			
		RestoreArchetypeEntities(EntityManagerLock, EntityManager, ArchetypeSnapshot, ArchetypeRestoreCache);
	});

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore ReleaseReservedEntity")));

		for (const auto& ArchetypeRestoreCache : ArchetypeRestoreCaches)
		{
			// Release all fill entities
			for (const FMassEntityHandle& FillEntity : ArchetypeRestoreCache->FillEntities)
			{
				EntityManager.DestroyEntity(FillEntity, false);
			}

			// Release all reserved entities
			for (const auto& ReservedEntity : ArchetypeRestoreCache->ReservedEntities)
			{
				EntityManager.ReleaseReservedEntity(ReservedEntity.Value);
			}
		}

		for (const auto& ReservedEntity : ReservedEntitiesCache)
		{
			EntityManager.ReleaseReservedEntity(ReservedEntity.Value);
		}
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore SetSharedFragments")));
		EntityManager.SetSharedFragments(Snapshot.SharedFragmentsMap);
	}
	
	EntityManager.SetSerialNumberGenerator(Snapshot.SerialNumberGenerator);
}

void InitArchetypeRestoreCache(FMassEntityManager& EntityManager, const TArray<FRecallArchetypeSnapshot>& ArchetypeSnapshots,
	TArray<TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreCaches,
	TMap<FMassArchetypeHandle, TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreMapCache)
{
	ArchetypeRestoreMapCache.Reserve(ArchetypeSnapshots.Num());
	ArchetypeRestoreCaches.SetNum(ArchetypeSnapshots.Num(), EAllowShrinking::No);

	for (int32 Index = 0; Index < ArchetypeSnapshots.Num(); Index++)
	{		
		const FRecallArchetypeSnapshot& ArchetypeSnapshot = ArchetypeSnapshots[Index];
		
		TSharedPtr<FRecallArchetypeRestoreCache> ArchetypeCache = MakeShared<FRecallArchetypeRestoreCache>();		
		ArchetypeCache->ArchetypeSnapshotIndex = Index;
		ArchetypeCache->ArchetypeSnapshot = &ArchetypeSnapshot;
		ArchetypeCache->Composition = ArchetypeSnapshot.Composition.Get();
		
		ArchetypeCache->ArchetypeHandle = EntityManager.CreateArchetype(
			ArchetypeCache->Composition);
		check(ArchetypeCache->ArchetypeHandle.IsValid());	

		ArchetypeRestoreMapCache.Add(ArchetypeCache->ArchetypeHandle, ArchetypeCache);
		ArchetypeRestoreCaches[Index] = ArchetypeCache;
	}
}
	
void PrepareMutableEntities(FMassEntityManager& EntityManager, int32 LastStaticEntitySerialNumber,
	const FRecallEntityManagerSnapshot& Snapshot, const TMap<FMassArchetypeHandle,
	TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreMapCache)
{
	const TArray<FMassEntityHandle> OldEntities = EntityManager.GetAllEntities();
	for (const FMassEntityHandle& OldEntity : OldEntities)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore MutableEntityCache")));

		// Skip static entities
		if (OldEntity.SerialNumber <= LastStaticEntitySerialNumber)
		{
			continue;
		}
		// Is newer than the last entity at that time
		else if (OldEntity.SerialNumber >= Snapshot.SerialNumberGenerator)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore DestroyEntity")));
			EntityManager.DestroyEntity(OldEntity, false);
		}
		else
		{
			TSharedPtr<FRecallArchetypeRestoreCache> ArchetypeCache;
					
			const FMassArchetypeHandle ArchetypeHandle = EntityManager.GetArchetypeForEntity(OldEntity);
			if (ensure(ArchetypeHandle.IsValid()))
			{
				// Look for an existing archetype from the snapshot
				ArchetypeCache = ArchetypeRestoreMapCache.FindRef(ArchetypeHandle);
			}

			// Archetype has changed
			if (!ArchetypeCache.IsValid() || !ensure(ArchetypeCache->ArchetypeSnapshotIndex != INDEX_NONE))
			{
				TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore DestroyEntity")));
				EntityManager.DestroyEntity(OldEntity, false);
			}
			else
			{
				TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore AddMutableEntity")));

				// Cache the mutable entity that could potentially be re-used.
				// It is using the same archetype as the snapshot, but its internal index must be validated later on.
				FRecallEntityRestoreMutableCache& NewEntityCache = ArchetypeCache->MutableEntities.AddDefaulted_GetRef();
				NewEntityCache.EntityHandle = OldEntity;
				NewEntityCache.ArchetypeHandle = ArchetypeHandle;
				NewEntityCache.ArchetypeSnapshot = ArchetypeCache->ArchetypeSnapshot;
			}
		}
	}
}

static void CheckIfEntityStillExists(const FMassEntityManager& EntityManager, FRecallEntityRestoreMutableCache& EntityCache)
{
	const FMassEntityHandle& Entity = EntityCache.EntityHandle;

	// Refresh the entity's absolute and chunk index
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore GetArchetypeInternalIndexForEntity")));

		const FMassArchetypeHandle& ArchetypeHandle = EntityCache.ArchetypeHandle;
		EntityManager.GetArchetypeInternalIndexForEntity(Entity, ArchetypeHandle, EntityCache.AbsoluteIndex, EntityCache.ChunkIndex);
	}

	checkf(EntityCache.ArchetypeSnapshot != nullptr,
		TEXT("%hs Archetype snapshot should exist so the entity can be restored"), __FUNCTION__);

	// Verify that the entity is still at the same location in the archetype.
	if (EntityCache.ArchetypeSnapshot->ChunkSnapshots.IsValidIndex(EntityCache.ChunkIndex))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore FindEntitySnapshot")));
		const FRecallChunkSnapshot& ChunkSnapshot = EntityCache.ArchetypeSnapshot->ChunkSnapshots[EntityCache.ChunkIndex];
		const TArray<FRecallEntitySnapshot>& EntitySnapshots = ChunkSnapshot.EntitySnapshots;
		EntityCache.bStillExists = EntitySnapshots.ContainsByPredicate(
			[&Entity, &EntityCache](const FRecallEntitySnapshot& EntitySnapshot)
		{
			return EntitySnapshot.AbsoluteIndex == EntityCache.AbsoluteIndex
				&& EntitySnapshot.Entity == Entity;
		});
	}
	else
	{
		EntityCache.bStillExists = false;
	}
}
	
void PrepareArchetypeRestoreCache(FCriticalSection& EntityManagerLock, FMassEntityManager& EntityManager,
	FRecallArchetypeRestoreCache& EntityArchetypeCache)
{
	TArray<FRecallEntityRestoreMutableCache>& MutableEntities = EntityArchetypeCache.MutableEntities;
	
	// Update absolute and chunk indices for entities
	for (FRecallEntityRestoreMutableCache& EntityCache : MutableEntities)
	{
		CheckIfEntityStillExists(EntityManager, EntityCache);
	}
	
	// Order the entities by absolute index
	// This way, if an entity at the front of the chunk is destroyed, then the next entities won't have the same absolute index anymore
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore Sort")));
		MutableEntities.Sort(
			[](const FRecallEntityRestoreMutableCache& lEntity, const FRecallEntityRestoreMutableCache& rEntity)
			{
				check(lEntity.ArchetypeHandle == rEntity.ArchetypeHandle);
				return lEntity.AbsoluteIndex < rEntity.AbsoluteIndex;
			}
		);
	}

	TSet<int32>& ExistingEntityAbsoluteIndices = EntityArchetypeCache.ExistingEntityAbsoluteIndices;
	
	int32 PreviousChunkIndex = INDEX_NONE;
	bool bDirtyChunk = false;
	
#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
	// Validate previous entities
	int32 ChunkEntityStartIndex = 0;

	int32 RefreshChunkUntilEntityIndex = INDEX_NONE;
#endif // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG

	// Destroy entities that have changed since the snapshot
	for (int32 EntityIndex = 0; EntityIndex < MutableEntities.Num(); EntityIndex++)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore DestroyChangedEntities")));

		FRecallEntityRestoreMutableCache& EntityCache = MutableEntities[EntityIndex];
		const FMassEntityHandle& OldEntity = EntityCache.EntityHandle;

#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		// While validating entities inside a dirty chunk, skip destroyed entities
		if (RefreshChunkUntilEntityIndex != INDEX_NONE)
		{
			if (!EntityManager.IsEntityValid(OldEntity))
			{
				if (EntityIndex == RefreshChunkUntilEntityIndex)
				{
					RefreshChunkUntilEntityIndex = INDEX_NONE;
				}
				continue;
			}
		}

		checkf(EntityManager.IsEntityValid(OldEntity),
			TEXT("%hs Entity must valid"), __FUNCTION__);
		
		int32 OldAbsoluteIndex = EntityCache.AbsoluteIndex;
		int32 OldChunkIndex = EntityCache.ChunkIndex;
#endif // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		
		checkf(EntityCache.ChunkIndex >= PreviousChunkIndex,
			TEXT("%hs Should never go back to previous chunks"), __FUNCTION__);
		
		// Keep track of the current chunk.
		if (PreviousChunkIndex != EntityCache.ChunkIndex)
		{
#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
			checkf(RefreshChunkUntilEntityIndex == INDEX_NONE,
				TEXT("%hs Should not change chunk while validating dirty chunk"), __FUNCTION__);
			
			ChunkEntityStartIndex = EntityIndex;
#endif // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
			
			PreviousChunkIndex = EntityCache.ChunkIndex;
			bDirtyChunk = false;
		}

#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		// Re-validate entities inside a dirty chunk
		if (RefreshChunkUntilEntityIndex != INDEX_NONE)
		{
			CheckIfEntityStillExists(EntityManager, EntityCache);
			
			ensureAlwaysMsgf(OldAbsoluteIndex == EntityCache.AbsoluteIndex && OldChunkIndex == EntityCache.ChunkIndex,
				TEXT("%hs Previous entities should not move when an entity is destroyed"), __FUNCTION__);
		}
#endif // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		
#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		if ((!bDirtyChunk || RefreshChunkUntilEntityIndex != INDEX_NONE) && EntityCache.bStillExists)
#else // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		if (!bDirtyChunk && EntityCache.bStillExists)
#endif
		{
			check(EntityCache.ChunkIndex == PreviousChunkIndex);
			ExistingEntityAbsoluteIndices.Add(EntityCache.AbsoluteIndex);
		}
		else
		{
			{
				TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore DestroyEntity")));
				FScopeLock ScopeLock(&EntityManagerLock);
				EntityManager.DestroyEntity(OldEntity, false);
			}

			bDirtyChunk = true;
			
#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
			checkf(RefreshChunkUntilEntityIndex == INDEX_NONE,
				TEXT("%hs RefreshChunkUntilEntityIndex was not reset"), __FUNCTION__);
			
			RefreshChunkUntilEntityIndex = EntityIndex;
			EntityIndex = ChunkEntityStartIndex - 1;
#endif // WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
		}
	}
}

void ReserveRestoreEntities(FMassEntityManager& EntityManager,
	const TArray<FRecallArchetypeSnapshot>& ArchetypeSnapshots, const TArray<TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreCaches,
	int32 SnapshotLastEntityIndex, TMap<int32, FMassEntityHandle>& ReservedEntitiesCache)
{
	int32 LastEntityIndex = INDEX_NONE;
	while (LastEntityIndex < SnapshotLastEntityIndex)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore ReserveEntity")));
		
		const FMassEntityHandle NewEntity = EntityManager.ReserveEntity();
		
		checkf(NewEntity.Index > LastEntityIndex,
			TEXT("%hs Entities should be ordered"), __FUNCTION__);
		
		LastEntityIndex = NewEntity.Index;
		ReservedEntitiesCache.Add(NewEntity.Index, NewEntity);
	}
	
	// Assign reserved entities to each archetype cache
	for (int32 ArchetypeIndex = 0; ArchetypeIndex < ArchetypeSnapshots.Num(); ArchetypeIndex++)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore ArchetypeRestoreCache")));

		const FRecallArchetypeSnapshot& ArchetypeSnapshot = ArchetypeSnapshots[ArchetypeIndex];
		FRecallArchetypeRestoreCache& ArchetypeRestoreCache = *ArchetypeRestoreCaches[ArchetypeIndex];

		// Reserve entities that will be restored
		for (const FRecallChunkSnapshot& ChunkSnapshot : ArchetypeSnapshot.ChunkSnapshots)
		{
			for (const FRecallEntitySnapshot& EntitySnapshot : ChunkSnapshot.EntitySnapshots)
			{
				if (EntitySnapshot.bMutable && !ArchetypeRestoreCache.ExistingEntityAbsoluteIndices.Contains(EntitySnapshot.AbsoluteIndex))
				{
					ArchetypeRestoreCache.ReservedEntities.Add(EntitySnapshot.Entity.Index,
						ReservedEntitiesCache.FindAndRemoveChecked(EntitySnapshot.Entity.Index));
				}
			}
		}
	}
}

#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
static void ValidateRestoredEntity(const FMassEntityManager& EntityManager,
	const FMassArchetypeHandle& ArchetypeHandle, const TSet<int32>& ExistingEntityAbsoluteIndices,
	const FRecallEntitySnapshot& EntitySnapshot, int32 ChunkIndex,
	const FMassEntityHandle& OldEntity, const FMassEntityHandle& NewEntity, bool bIsExisting)
{	
	int32 NewAbsoluteIndex = INDEX_NONE, NewChunkIndex = INDEX_NONE;
	EntityManager.GetArchetypeInternalIndexForEntity(NewEntity, ArchetypeHandle, NewAbsoluteIndex, NewChunkIndex);
	
	ensureAlwaysMsgf(NewAbsoluteIndex == EntitySnapshot.AbsoluteIndex && ChunkIndex == NewChunkIndex,
		TEXT("%hs Entities should keep the same internal order"), __FUNCTION__);
	ensureAlwaysMsgf(NewEntity == OldEntity,
		TEXT("%hs Entities should be persistent between rollback"), __FUNCTION__);

	if (bIsExisting)
	{
		checkf(ExistingEntityAbsoluteIndices.Contains(NewAbsoluteIndex),
			TEXT("%hs Entity should already exist"), __FUNCTION__);
	}
}
#endif // WITH_MS_RESTORE_ENTITY_DEBUG
	
void RestoreArchetypeEntities(FCriticalSection& EntityManagerLock, FMassEntityManager& EntityManager,
	const FRecallArchetypeSnapshot& ArchetypeSnapshot, FRecallArchetypeRestoreCache& ArchetypeRestoreCache)
{	
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore Archetype")));

	const FMassArchetypeHandle& ArchetypeHandle = ArchetypeRestoreCache.ArchetypeHandle;
	const TSet<int32>& ExistingEntityAbsoluteIndices = ArchetypeRestoreCache.ExistingEntityAbsoluteIndices;

	for (int32 ChunkIndex = 0; ChunkIndex < ArchetypeSnapshot.ChunkSnapshots.Num(); ChunkIndex++)
	{
		const FRecallChunkSnapshot& ChunkSnapshot = ArchetypeSnapshot.ChunkSnapshots[ChunkIndex];
		const FMassArchetypeSharedFragmentValues SharedFragmentValues = ChunkSnapshot.SharedFragmentValues.Get(EntityManager);

		// Skip empty chunks.
		if (ChunkSnapshot.EntitySnapshots.Num() == 0)
		{
			continue;
		}
		
		for (const FRecallEntitySnapshot& EntitySnapshot : ChunkSnapshot.EntitySnapshots)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore Entity")));

			// We skip all the static entities. This is a bit risky, but this is a major optimization.
			if (EntitySnapshot.bMutable)
			{
				const FMassEntityHandle& OldEntity = EntitySnapshot.Entity;
				FMassEntityHandle NewEntity = OldEntity;

				if (ArchetypeRestoreCache.ReservedEntities.RemoveAndCopyValue(OldEntity.Index, NewEntity))
				{
					EntityManager.RestoreEntitySerialNumber(NewEntity, OldEntity.SerialNumber);
					EntityManager.BuildEntity(NewEntity, ArchetypeHandle, SharedFragmentValues, false, ChunkIndex);
					
#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
					ValidateRestoredEntity(EntityManager, ArchetypeHandle, ExistingEntityAbsoluteIndices,
						EntitySnapshot, ChunkIndex, OldEntity, NewEntity, false);
#endif // WITH_MS_RESTORE_ENTITY_DEBUG
				}
#if WITH_RECALL_ENTITY_SNAPSHOT_DEBUG
				else
				{
					checkf(EntityManager.GetArchetypeForEntity(NewEntity) == ArchetypeHandle,
						TEXT("%hs Archetype should match"), __FUNCTION__);
					
					ValidateRestoredEntity(EntityManager, ArchetypeHandle, ExistingEntityAbsoluteIndices,
						EntitySnapshot, ChunkIndex, OldEntity, NewEntity, true);
				}
#endif // WITH_MS_RESTORE_ENTITY_DEBUG

				{
					TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore SetEntityFragmentsValues")));
					EntityManager.SetEntityFragmentValues(NewEntity, EntitySnapshot.FragmentInstanceList);
				}
			}
		}
	}	
}
	
} // Recall::EntitySnapshot::Restore::Utils
