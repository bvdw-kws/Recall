// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassArchetypeTypes.h"
#include "Mass/EntityHandle.h"

#define WITH_RECALL_ENTITY_SNAPSHOT_DEBUG ((UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT) && 0)

struct FRecallArchetypeSnapshot;

struct FRecallEntitySaveCache
{
	FMassEntityHandle Entity;
	int32 AbsoluteIndex{ INDEX_NONE };
	int32 ChunkIndex{ INDEX_NONE };
};

struct FRecallArchetypeSaveCache
{
	TArray<FRecallEntitySaveCache> Entities;

	void ResetCache()
	{
		Entities.Reset();
	}
};

struct FRecallEntityRestoreSaveCacheManager
{
	TMap<FMassArchetypeHandle, FRecallArchetypeSaveCache> ArchetypeCacheMap;
	TArray<FMassArchetypeHandle> ArchetypeHandles;

	void ResetCache()
	{
		ArchetypeHandles.Reset();

		for (auto& ArchetypeCache : ArchetypeCacheMap)
		{
			ArchetypeCache.Value.ResetCache();
		}
	}
};

/*
* Cache of a mutable entity to evaluate if we should destroy or restore it
*/
struct FRecallEntityRestoreMutableCache
{
	FMassEntityHandle EntityHandle;
	FMassArchetypeHandle ArchetypeHandle;
	const FRecallArchetypeSnapshot* ArchetypeSnapshot = nullptr;
	int32 AbsoluteIndex = INDEX_NONE;
	int32 ChunkIndex = INDEX_NONE;
	bool bStillExists = true;
};

/**
* Cache used in thread to restore our archetype
*/
struct FRecallArchetypeRestoreCache
{
	FMassArchetypeHandle ArchetypeHandle;
	const FRecallArchetypeSnapshot* ArchetypeSnapshot = nullptr;
	FMassArchetypeCompositionDescriptor Composition;
	int32 ArchetypeSnapshotIndex = INDEX_NONE;
	TArray<FRecallEntityRestoreMutableCache> MutableEntities;
	TSet<int32> ExistingEntityAbsoluteIndices;
	TMap<int32, FMassEntityHandle> ReservedEntities;
	TArray<FMassEntityHandle> FillEntities;
};

struct FRecallEntityRestoreCacheManager
{
	TMap<FMassArchetypeHandle, TSharedPtr<FRecallArchetypeRestoreCache>> ArchetypeRestoreMap;
	TArray<TSharedPtr<FRecallArchetypeRestoreCache>> ArchetypeRestoreCaches;
	TMap<int32, FMassEntityHandle> ReservedEntities;

	void ResetCache()
	{
		ArchetypeRestoreMap.Reset();
		ArchetypeRestoreCaches.Reset();
		ReservedEntities.Reset();
	}
};
