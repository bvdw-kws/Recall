// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FMassArchetypeHandle;
struct FMassEntityHandle;
struct FMassEntityManager;
struct FRecallArchetypeRestoreCache;
struct FRecallArchetypeSnapshot;
struct FRecallEntityManagerSnapshot;

namespace Recall::EntitySnapshot::Restore::Utils
{
	
extern void RestoreEntityManager(FMassEntityManager& EntityManager, const FRecallEntityManagerSnapshot& Snapshot,
	int32 LastStaticEntitySerialNumber,	TMap<int32, FMassEntityHandle>& ReservedEntitiesCache,
	TMap<FMassArchetypeHandle, TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreMapCache,
	TArray<TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreCaches);

/**
 * Initialize the archetype cache with its composition and the archetype handle.
 */
extern void InitArchetypeRestoreCache(FMassEntityManager& EntityManager, const TArray<FRecallArchetypeSnapshot>& ArchetypeSnapshots,
	TArray<TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreCaches,
	TMap<FMassArchetypeHandle, TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreMapCache);
	
/**
 * Gather mutable entities that could potentially be restored
 */
extern void PrepareMutableEntities(FMassEntityManager& EntityManager, int32 LastStaticEntitySerialNumber,
	const FRecallEntityManagerSnapshot& Snapshot, const TMap<FMassArchetypeHandle,
	TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreMapCache);

/**
 * Validate the absolute index of all the mutable entities that could potentially be re-used.
 */
extern void PrepareArchetypeRestoreCache(FCriticalSection& EntityManagerLock, FMassEntityManager& EntityManager,
	FRecallArchetypeRestoreCache& EntityArchetypeCache);
	
extern void ReserveRestoreEntities(FMassEntityManager& EntityManager,
	const TArray<FRecallArchetypeSnapshot>& ArchetypeSnapshots, const TArray<TSharedPtr<FRecallArchetypeRestoreCache>>& ArchetypeRestoreCaches,
	int32 SnapshotLastEntityIndex, TMap<int32, FMassEntityHandle>& ReservedEntitiesCache);
extern void RestoreArchetypeEntities(FCriticalSection& EntityManagerLock, FMassEntityManager& EntityManager,
	const FRecallArchetypeSnapshot& ArchetypeSnapshot, FRecallArchetypeRestoreCache& ArchetypeRestoreCache);
	
} // Recall::EntitySnapshot::Restore::Utils
