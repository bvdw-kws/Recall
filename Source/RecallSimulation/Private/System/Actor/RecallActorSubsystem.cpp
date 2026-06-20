// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Actor/RecallActorSubsystem.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/DecalActor.h"
#include "Engine/StaticMeshActor.h"
#include "LevelSequenceActor.h"
#include "Representation/Actor/Factory/RecallActorRepresentationPoolFactory.h"
#include "Representation/Actor/Pool/RecallObjectPoolTypes.h"
#include "Representation/Actor/RecallActorRepresentationTypes.h"
#include "Serialization/ArchiveCrc32.h"
#include "System/Simulation/RecallSimulationSubsystem.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

static uint32 GetStructCrc32(const FInstancedStruct& Struc, const uint32 CRC = 0)
{
	const UScriptStruct& ScriptStruct = *Struc.GetScriptStruct();
	const FString StructPathName = ScriptStruct.GetPathName();

	// MurmurFinalize32
	uint32 StructHash = GetTypeHash(StructPathName);
	StructHash ^= StructHash >> 16;
	StructHash *= 0x85ebca6b;
	StructHash ^= StructHash >> 13;
	StructHash *= 0xc2b2ae35;
	StructHash ^= StructHash >> 16;

	FArchiveCrc32 Ar(HashCombine(CRC, StructHash));
	UScriptStruct& NonConstScriptStruct = const_cast<UScriptStruct&>(ScriptStruct);
	NonConstScriptStruct.SerializeItem(Ar, const_cast<uint8*>(Struc.GetMemory()), nullptr);

	const uint32 Hash = Ar.GetCrc();
	return Hash;
}

URecallActorSubsystem::URecallActorSubsystem()
	: Super()
{
}

void URecallActorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallSimulationSubsystem>();

	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnPreRender.AddUObject(this, &ThisClass::OnPreRender);
	}

	PoolQueue.SetNum(RECALL_OBJECT_POOL_MAX);

	for (int32 PoolIndex = 0; PoolIndex < RECALL_OBJECT_POOL_MAX; PoolIndex++)
	{
		PoolQueue[PoolIndex] = NewObject<URecallObjectPoolContainer>(this);
	}
}

void URecallActorSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnPreRender.RemoveAll(this);
	}

#if !UE_BUILD_SHIPPING
	for (const TPair<FName, TObjectPtr<URecallObjectPoolContainer>>& PoolContainer : PoolContainerMap)
	{
		if (PoolContainer.Value)
		{
			PoolContainer.Value->StopTrackAllObjects();
		}
	}
#endif // !UE_BUILD_SHIPPING
}

void URecallActorSubsystem::Reset()
{
	// Release our active actors from the pool.
	for (const auto& ClassNameToActorPool : ClassNameToActorPoolMap)
	{
		if (ClassNameToActorPool.Value.IsValid())
		{
			ClassNameToActorPool.Value->ReleaseAllObjects();
		}
	}
	
	// Reset our registry.
	ActorRegistry = FRecallActorRegistry();
}

void URecallActorSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallActorSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Actor_Save);

	OutSnapshot.InitializeAs<FRecallActorRegistry>(ActorRegistry);
}

void URecallActorSubsystem::PreRestore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallActorSubsystem::PreRestore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Actor_PreRestore);

	if (const FRecallActorRegistry* InData = InSnapshot.GetPtr<FRecallActorRegistry>())
	{
		ActorRegistry = *InData;

		TArray<FRecallActorHandle> ActorHandles;
		CleanupActorsFromPool(ActorHandles);
		RestoreActorsFromPool(ActorHandles);
	}
}

void URecallActorSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallActorSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Actor_Restore);

	// Actor restoration now happens in PreRestore to avoid parallel execution race conditions
	// This method is kept for interface compliance but does no work
}

void URecallActorSubsystem::OnPreRender()
{
	// Flush these commands before rendering the simulation so the actors are ready.
	FlushActorPoolCommands();
}

void URecallActorSubsystem::FlushActorPoolCommands()
{
	FScopeLock Lock(&DataGuard);
	for (auto& ClassNameToActorPool : ClassNameToActorPoolMap)
	{
		if (ClassNameToActorPool.Value.IsValid())
		{
			ClassNameToActorPool.Value->FlushCommands();
		}
	}
}

void URecallActorSubsystem::CleanupActorsFromPool(TArray<FRecallActorHandle>& OutHandles)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallActorSubsystem::CleanupActorsFromPool"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Actor_CleanupActorsFromPool);
	
	TSet<FRecallActorHandle> OldActorHandles;
	ActorRegistry.HandleToInstanceHash.GetKeys(OldActorHandles);

	for (const TPair<FName, TSharedPtr<IRecallObjectPoolInterface>>& ClassNameToActorPool : ClassNameToActorPoolMap)
	{
		checkf(ClassNameToActorPool.Value.IsValid(), TEXT("Invalid pool"));

		// Copy our active actor list since it will be modified below.
		const TArray<int32> ActiveActorIndices = ClassNameToActorPool.Value->GetActiveIndices();

		for (const int32 ActorIndex : ActiveActorIndices)
		{
			const FRecallActorHandle Handle = ClassNameToActorPool.Value->GetOwnerHandleAtIndex(ActorIndex);

			checkf(Handle.IsValid(), TEXT("%hs Handle is not valid."), __FUNCTION__);

			if (OldActorHandles.Contains(Handle))
			{
				// Ignore actors that have remained the same.
				OldActorHandles.Remove(Handle);
			}
			else
			{
				// Release actors that have changed.
				ReleaseActorFromPool(Handle);
			}
		}
	}

	OutHandles = OldActorHandles.Array();
}

void URecallActorSubsystem::RestoreActorsFromPool(const TArray<FRecallActorHandle>& ActorHandles)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallActorSubsystem::RestoreActorsFromPool"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Actor_RestoreActorsFromPool);
	
	for (const FRecallActorHandle& ActorHandle : ActorHandles)
	{
		const uint32 Hash = ActorRegistry.HandleToInstanceHash.FindChecked(ActorHandle);
		const FInstancedStruct& EntryInstance = ActorRegistry.HashToEntryInstance.FindChecked(Hash).InstancedStruct;
		
		FRecallActorHandle DummyHandle;
		CreateOrRestoreActor_Internal(EntryInstance, DummyHandle, &ActorHandle);
	}
}

FRecallActorHandle URecallActorSubsystem::CreateActor_Internal(const FInstancedStruct& Desc)
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(GetWorld());

	FRecallActorHandle Handle;
	CreateOrRestoreActor_Internal(Desc, Handle, nullptr);
	return Handle;
}

void URecallActorSubsystem::CreateOrRestoreActor_Internal(const FInstancedStruct& Desc,
	FRecallActorHandle& OutHandle, const FRecallActorHandle* RestoreHandle)
{
	const TSharedPtr<IRecallObjectPoolInterface> Pool = GetOrCreatePool(Desc);
	check(Pool.IsValid());
	
	OutHandle = Pool->RequestObject(Desc, RestoreHandle);
	
	if (OutHandle.IsValid())
	{
		StoreEntryInstance_Internal(OutHandle, Desc);
	}
}

TWeakObjectPtr<AActor> URecallActorSubsystem::GetActor_Internal(const FRecallActorHandle& Handle) const
{	
	if (Handle.IsValid())
	{
		const FScopeLock Lock(&const_cast<URecallActorSubsystem*>(this)->DataGuard);

		if (const TSharedPtr<IRecallObjectPoolInterface>* Pool = ClassNameToActorPoolMap.Find(Handle.ClassName))
		{
			return Pool->Get()->GetObject(Handle);
		}
	}

	return nullptr;
}

void URecallActorSubsystem::StoreEntryInstance_Internal(const FRecallActorHandle& ActorHandle,
                                                          const FInstancedStruct& Instance)
{
	const uint32 Hash = GetStructCrc32(Instance);
	ActorRegistry.HandleToInstanceHash.Add(ActorHandle, Hash);
	
	FRecallActorInstanceEntry& InstanceEntry = ActorRegistry.HashToEntryInstance.FindOrAdd(Hash);
	if (InstanceEntry.Count == 0)
	{
		InstanceEntry.InstancedStruct = Instance;
	}
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	else
	{
		checkf(InstanceEntry.InstancedStruct.GetScriptStruct() == Instance.GetScriptStruct(),
			TEXT("Must be of the same type"));
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	InstanceEntry.Count++;
}

void URecallActorSubsystem::ReleaseActor(FRecallActorHandle& ActorHandle)
{
	if (!ActorHandle.IsValid())
	{
		return;
	}

	Recall::Simulation::Utils::CheckSimulationProcessingPhase(GetWorld());

	// Release entry from our registry.
	uint32 Hash = 0;
	ActorRegistry.HandleToInstanceHash.RemoveAndCopyValue(ActorHandle, Hash);
	FRecallActorInstanceEntry& InstanceEntry = ActorRegistry.HashToEntryInstance[Hash];
	if (--InstanceEntry.Count == 0)
	{
		ActorRegistry.HashToEntryInstance.Remove(Hash);
	}
	
	ReleaseActorFromPool(ActorHandle);

	ActorHandle.Invalidate();
}

void URecallActorSubsystem::ReleaseActorFromPool(const FRecallActorHandle& ActorHandle)
{
	FScopeLock Lock(&DataGuard);

	const TSharedPtr<IRecallObjectPoolInterface>* ActorPool = ClassNameToActorPoolMap.Find(ActorHandle.ClassName);

	if (ensureMsgf(ActorPool, TEXT("%hs We don't have a Pool for this actor class"), __FUNCTION__))
	{
		check(ActorPool->IsValid());

		ActorPool->Get()->ReleaseObject(ActorHandle);
	}
}

TSharedPtr<IRecallObjectPoolInterface> URecallActorSubsystem::GetOrCreatePool(const FInstancedStruct& Desc)
{
	const FRecallActorRepresentationDesc& ActorRepresentationDesc = Desc.Get<FRecallActorRepresentationDesc>();
	check(ActorRepresentationDesc.FactoryClass);
	
	const URecallActorRepresentationPoolFactory* FactoryCDO = CastChecked<URecallActorRepresentationPoolFactory>(
		ActorRepresentationDesc.FactoryClass->GetDefaultObject());
	check(FactoryCDO);
	
	const FName ClassName = FactoryCDO->GetActorPoolName(Desc);
	checkf(!ClassName.IsNone(), TEXT("%hs We don't have a valid ClassName for this factory class: %s"), __FUNCTION__,
		*FactoryCDO->GetClass()->GetName());

	FScopeLock Lock(&DataGuard);

	if (const TSharedPtr<IRecallObjectPoolInterface>* Pool = ClassNameToActorPoolMap.Find(ClassName))
	{
		return *Pool;
	}
	else
	{
		// Use index-based approach instead of Pop() to avoid container reuse issues
		checkf(NextFreePoolIndex < PoolQueue.Num(), 
			TEXT("%hs Ran out of pre-allocated pool containers. Current=%d, Max=%d"), 
			__FUNCTION__, NextFreePoolIndex, PoolQueue.Num());
		
		URecallObjectPoolContainer* Container = PoolQueue[NextFreePoolIndex++];
		
		// Check container state before initialization
		checkf(Container, TEXT("%hs PoolQueue[%d] returned nullptr container"), __FUNCTION__, NextFreePoolIndex - 1);
		
		// Get soft class path for async loading
		const FSoftClassPath SoftClassPath = ActorRepresentationDesc.GetSoftClassPath();
		checkf(!SoftClassPath.IsNull(), TEXT("%hs Must have a valid class"), __FUNCTION__);
		
		// Initialize container for async loading
		Container->InitContainerAsync(ClassName, SoftClassPath, SerialNumberGenerator);
		
		// Verify InitContainer actually set the ClassName
		checkf(!Container->GetClassName().IsNone() && Container->GetClassName() == ClassName, 
			TEXT("%hs InitContainer set wrong ClassName: Expected='%s', Got='%s'"), 
			__FUNCTION__, *ClassName.ToString(), *Container->GetClassName().ToString());

		// Keep track of our container using UPROPERTY
		PoolContainerMap.Add(ClassName, Container);

		const TSharedPtr<IRecallObjectPoolInterface> NewPool = FactoryCDO->BuildActorPool();
		NewPool->SetPoolContainer(Container);
		ClassNameToActorPoolMap.Add(ClassName, NewPool);

		return NewPool;
	}
}

TWeakObjectPtr<ASkeletalMeshActor> URecallActorSubsystem::GetSkeletalMeshActor(
	const FRecallActorHandle& Handle) const
{
	return GetActor<ASkeletalMeshActor>(Handle);
}

TWeakObjectPtr<AStaticMeshActor> URecallActorSubsystem::GetStaticMeshActor(
	const FRecallActorHandle& Handle) const
{
	return GetActor<AStaticMeshActor>(Handle);
}

TWeakObjectPtr<ADecalActor> URecallActorSubsystem::GetDecalActor(const FRecallActorHandle& Handle) const
{
	return GetActor<ADecalActor>(Handle);
}

TWeakObjectPtr<ALevelSequenceActor> URecallActorSubsystem::GetLevelSequenceActor(
	const FRecallActorHandle& Handle) const
{
	return GetActor<ALevelSequenceActor>(Handle);
}
