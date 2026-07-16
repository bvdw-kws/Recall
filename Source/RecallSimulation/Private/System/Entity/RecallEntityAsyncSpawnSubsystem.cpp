// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Entity/RecallEntityAsyncSpawnSubsystem.h"

#include "Desync/RecallDesyncLog.h"
#include "MassEntityConfigAsset.h"
#include "MassExecutionContext.h"
#include "RecallEntityAsyncSpawnSnapshotTypes.h"
#include "System/Asset/RecallAssetManagerSubsystem.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

URecallEntityAsyncSpawnSubsystem::URecallEntityAsyncSpawnSubsystem()
	: Super()
{
}

void URecallEntityAsyncSpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallAssetManagerSubsystem>();
	Collection.InitializeDependency<URecallEntitySubsystem>();

	AssetManagerSystem = UWorld::GetSubsystem<URecallAssetManagerSubsystem>(GetWorld());
	EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(GetWorld());

	SpawnQueue = MakeShared<FRecallEntityAsyncSpawnQueue>();
}

void URecallEntityAsyncSpawnSubsystem::Deinitialize()
{
	Super::Deinitialize();
	
	AssetManagerSystem.Reset();
	EntitySystem.Reset();
	
	SpawnQueue.Reset();
}

void URecallEntityAsyncSpawnSubsystem::Start(const FRecallSimulationStartParams& Params)
{
}

void URecallEntityAsyncSpawnSubsystem::Reset()
{
	GetMutableSpawnQueue().Reset();
}

void URecallEntityAsyncSpawnSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallEntityAsyncSpawnSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_EntityAsyncSpawn_Save);

	checkf(!SpawnContext.IsValid(), TEXT("Spawn context should not be set outside of entity creation."));

	check(SpawnQueue.IsValid());
	OutSnapshot.InitializeAs<FRecallEntityAsyncSpawnQueue>((*SpawnQueue.Get()));
}

void URecallEntityAsyncSpawnSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallEntityAsyncSpawnSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_EntityAsyncSpawn_Restore);

	if (const FRecallEntityAsyncSpawnQueue* DataPtr = InSnapshot.GetPtr<FRecallEntityAsyncSpawnQueue>())
	{
		check(SpawnQueue.IsValid());
		(*SpawnQueue.Get()) = *DataPtr;
	}
}

FRecallEntityAsyncSpawnQueue& URecallEntityAsyncSpawnSubsystem::GetMutableSpawnQueue()
{
	check(SpawnQueue.IsValid());
	return *SpawnQueue.Get();
}

const FRecallEntityAsyncSpawnQueue& URecallEntityAsyncSpawnSubsystem::GetSpawnQueue() const
{
	return const_cast<URecallEntityAsyncSpawnSubsystem*>(this)->GetMutableSpawnQueue();
}

void URecallEntityAsyncSpawnSubsystem::SpawnEntityAsync(
	const TSoftObjectPtr<UMassEntityConfigAsset>& EntityConfigAsset, const FVector& Position, FQuat Rotation,
	TOptional<FRecallEntityAsyncSpawnParameters> Params)
{
	SpawnEntityAsync(EntityConfigAsset.ToSoftObjectPath(), Position, Rotation, Params);
}

void URecallEntityAsyncSpawnSubsystem::SpawnEntityAsync(const FSoftObjectPath& EntityConfigAsset,
	const FVector& Position, FQuat Rotation, TOptional<FRecallEntityAsyncSpawnParameters> Params)
{
	if (EntityConfigAsset.IsNull())
	{
		return;
	}
	
	const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);
	
	FScopeLock Lock(&DataGuard);
	
	FRecallEntityAsyncSpawnRequest& NewRequest = GetMutableSpawnQueue().Requests.AddDefaulted_GetRef();
	NewRequest.RequestFrame = Frame;
	NewRequest.EntityConfigAsset = EntityConfigAsset;
	NewRequest.Position = Position;
	NewRequest.Rotation = Rotation;

	if (ensureAlwaysMsgf(AssetManagerSystem.IsValid(),
			TEXT("%hs Invalid asset manager system"), __FUNCTION__))
	{
		NewRequest.EntityConfigAssetHandle = AssetManagerSystem->RequestAsset(EntityConfigAsset);
	}

	if (Params.IsSet())
	{
		NewRequest.SpawnParameters = Params.GetValue();
	}
}

const FRecallEntityAsyncSpawnContext* URecallEntityAsyncSpawnSubsystem::PeekSpawnContext() const
{
	return SpawnContext.Get();
}

bool URecallEntityAsyncSpawnSubsystem::IsAnyRequestReady() const
{
	if (!ensure(AssetManagerSystem.IsValid()))
	{
		return false;
	}
	
	FScopeLock Lock(&DataGuard);

	for (const FRecallEntityAsyncSpawnRequest& Request : GetSpawnQueue().Requests)
	{
		if (AssetManagerSystem->IsAssetLoaded(Request.EntityConfigAssetHandle))
		{
			return true;
		}
	}
	
	return false;
}

void URecallEntityAsyncSpawnSubsystem::PushQueue(FMassEntityManager& System)
{
	TArray<FRecallEntityAsyncSpawnRequest>& Requests = GetMutableSpawnQueue().Requests;
	for (int32 RequestIndex = Requests.Num() - 1; RequestIndex >= 0; RequestIndex--)
	{
		const FRecallEntityAsyncSpawnRequest& Request = Requests[RequestIndex];
		if (SpawnRequestEntity(System, Request))
		{
			Requests.RemoveAtSwap(RequestIndex);
		}
	}
}

bool URecallEntityAsyncSpawnSubsystem::SpawnRequestEntity(FMassEntityManager& System,
	const FRecallEntityAsyncSpawnRequest& Request) const
{
	check(AssetManagerSystem.IsValid());

	if (!AssetManagerSystem->IsAssetLoaded(Request.EntityConfigAssetHandle))
	{
		return false;
	}
	
	FScopeLock Lock(&DataGuard);
	
	FRecallAssetLoadHandle EntityConfigAssetHandle = Request.EntityConfigAssetHandle;
	const auto* EntityConfigAsset = AssetManagerSystem->GetLoadedAsset<UMassEntityConfigAsset>(
		EntityConfigAssetHandle);

	if (EntitySystem.IsValid() && IsValid(EntityConfigAsset) && Request.SpawnParameters.EntityCount > 0)
	{
		SpawnContext = MakeUnique<FRecallEntityAsyncSpawnContext>();
		SpawnContext->Position = Request.Position;
		SpawnContext->Rotation = Request.Rotation;
		SpawnContext->SpawnParameters = Request.SpawnParameters;

		TArray<FMassEntityHandle> Entities;
		EntitySystem->CreateEntities(EntityConfigAsset, Request.SpawnParameters.EntityCount, Entities);

		SpawnContext.Reset();

		if (const auto* SpawnCommand = Request.SpawnParameters.SpawnCommand.GetPtr<FRecallEntityAsyncSpawnCommand>())
		{
			SpawnCommand->OnSpawn(System, Entities);
		}

#if RECALL_DESYNC_LOG
		RECALL_DESYNC_LOG_STR(this, Spawn,
			FString::Printf(TEXT("%s (count: %d)"),
				*EntityConfigAsset->GetFName().ToString(), Request.SpawnParameters.EntityCount));
		for (const FMassEntityHandle& Entity : Entities)
		{
			const FMassArchetypeHandle ArchetypeHandle = System.GetArchetypeForEntity(Entity);
			int32 AbsoluteIndex = INDEX_NONE, ChunkIndex = INDEX_NONE;
			System.GetArchetypeInternalIndexForEntity(Entity, ArchetypeHandle, AbsoluteIndex, ChunkIndex);
			RECALL_DESYNC_LOG_STR(this, Spawn, FString::Printf(TEXT("Entity: %s (AbsoluteIndex: %d, ChunkIndex: %d)"), *Entity.DebugGetDescription(), AbsoluteIndex, ChunkIndex));
		}
#endif // RECALL_DESYNC_LOG
	}

	AssetManagerSystem->ReleaseAsset(EntityConfigAssetHandle);
	return true;
}
