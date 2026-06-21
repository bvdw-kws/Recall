// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Asset/RecallAssetManagerSubsystem.h"

#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "Utility/Simulation/RecallSimulationUtils.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

URecallAssetManagerSubsystem::URecallAssetManagerSubsystem()
	: Super()
{
}

void URecallAssetManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallMultiSimSubsystem>();
	
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(this);

	if (URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		MultiSimSystem->GetOnTickStartEvent().AddUObject(this, &ThisClass::OnTickStart);
	}

	AssetQueue = NewObject<URecallAssetManagerGamethreadQueue>(this);
}

void URecallAssetManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(this);

	if (URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		MultiSimSystem->GetOnTickStartEvent().RemoveAll(this);
	}
	
	if (AssetQueue)
	{
		AssetQueue->ReleaseAllRunners();
		AssetQueue = nullptr;
	}
}

void URecallAssetManagerSubsystem::Reset()
{
	ClearAssetCache();
	
	if (AssetQueue)
	{
		AssetQueue->ReleaseAllRunners();
	}
}

void URecallAssetManagerSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallAssetManagerSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_AssetManager_Save);

	FScopeLock Lock(&DataGuard);
	OutSnapshot.InitializeAs<FRecallAssetManagerData>(AssetManagerData);
}

void URecallAssetManagerSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallAssetManagerSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_AssetManager_Restore);

	FScopeLock Lock(&DataGuard);
	AssetManagerData = InSnapshot.Get<FRecallAssetManagerData>();
	
	// Regenerate local cache from restored data
	RegenerateRequestMap();
}

void URecallAssetManagerSubsystem::OnTickStart(float DeltaTime)
{
	if (AssetQueue)
	{
		AssetQueue->UpdateAssetManagerRunners(AssetManagerData.AssetCache);
	}
}

FRecallAssetLoadHandle URecallAssetManagerSubsystem::RequestAsset(const FSoftObjectPath& Path, int32 LoadDuration)
{
	if (Path.IsNull())
	{
		return FRecallAssetLoadHandle();
	}

	const FRecallAssetLoadHandle Handle = FRecallAssetLoadHandle(++AssetManagerData.SerialNumberGenerator);

	{
		FScopeLock Lock(&DataGuard);
		
		// Check if asset is already cached
		FRecallAssetManagerLoadData* CachedData = AssetManagerData.AssetCache.Find(Path);
		if (CachedData)
		{
			// Increment reference count
			CachedData->ReferenceCount++;
			
			// Asset is already cached, use existing data
			RequestMap.Add(Handle, *CachedData);
			AssetManagerData.HandleToAssetPath.Add(Handle, Path);			
			
			UE_LOG(LogRecallAsset, Verbose,
				TEXT("%hs Asset: %s, RefCount=%d"), __FUNCTION__, *Path.ToString(), CachedData->ReferenceCount);
		}
		else
		{
			const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);
			const int32 MaxStepCount = Recall::Simulation::Utils::GetMaxStepCount(this);
		
			// Asset not cached, create new load data
			FRecallAssetManagerLoadData& Request = RequestMap.Add(Handle);
			Request.AsyncStartFrame = Frame;
			Request.AsyncEndFrame = Frame + MaxStepCount + LoadDuration;
			Request.AssetPath = Path;
			Request.ReferenceCount = 1;
			
			// Cache the asset data
			AssetManagerData.AssetCache.Add(Path, Request);
			AssetManagerData.HandleToAssetPath.Add(Handle, Path);
			UE_LOG(LogRecallAsset, Verbose,
				TEXT("%hs Asset: %s, RefCount=1 (new)"), __FUNCTION__, *Path.ToString());
		}
	}
	
	return Handle;
}

void URecallAssetManagerSubsystem::ReleaseAsset(FRecallAssetLoadHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	{
		FScopeLock Lock(&DataGuard);
		
		// Get the asset path for this handle
		const FSoftObjectPath* AssetPathPtr = AssetManagerData.HandleToAssetPath.Find(Handle);
		if (AssetPathPtr)
		{
			// Decrement reference count in cache
			FRecallAssetManagerLoadData* CachedData = AssetManagerData.AssetCache.Find(*AssetPathPtr);
			if (CachedData)
			{
				CachedData->ReferenceCount--;
				
				// Remove from cache if no more references
				if (CachedData->ReferenceCount <= 0)
				{
					AssetManagerData.AssetCache.Remove(*AssetPathPtr);
					
					UE_LOG(LogRecallAsset, Verbose,
						TEXT("%hs Asset: %s, RefCount=0 (removed)"), __FUNCTION__, *AssetPathPtr->ToString());
				}
				else
				{					
					UE_LOG(LogRecallAsset, Verbose,
						TEXT("%hs Asset: %s, RefCount=%d"), __FUNCTION__,
						*AssetPathPtr->ToString(), CachedData->ReferenceCount);
				}
			}
		
			// Remove from handle mapping
			AssetManagerData.HandleToAssetPath.Remove(Handle);
			RequestMap.Remove(Handle);
		}
	}
	
	Handle.Invalidate();
}

bool URecallAssetManagerSubsystem::IsAssetLoaded(const FRecallAssetLoadHandle& Handle) const
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);

	FScopeLock Lock(&DataGuard);
	const FRecallAssetManagerLoadData* RequestPtr = RequestMap.Find(Handle);
	if (ensureMsgf(RequestPtr != nullptr,
		TEXT("%hs Asset was not requested"), __FUNCTION__))
	{
		return Frame >= RequestPtr->AsyncEndFrame;
	}
	return false;
}

UObject* URecallAssetManagerSubsystem::GetLoadedAsset(const FRecallAssetLoadHandle& Handle) const
{
	if (ensure(Handle.IsValid()) && AssetQueue)
	{
		// Find the asset path for this handle
		const FSoftObjectPath* AssetPathPtr = AssetManagerData.HandleToAssetPath.Find(Handle);
		if (AssetPathPtr)
		{
			return AssetQueue->GetLoadedAsset(*AssetPathPtr);
		}
	}
	return nullptr;
}

void URecallAssetManagerSubsystem::ClearAssetCache()
{
	FScopeLock Lock(&DataGuard);
	AssetManagerData.Reset();
	RequestMap.Reset();
}

void URecallAssetManagerSubsystem::RegenerateRequestMap()
{
	FScopeLock Lock(&DataGuard);
	
	// Clear local cache
	RequestMap.Reset();
	
	// Regenerate from HandleToAssetPath mapping
	for (const TTuple<FRecallAssetLoadHandle, FSoftObjectPath>& HandlePair : AssetManagerData.HandleToAssetPath)
	{
		const FRecallAssetLoadHandle& Handle = HandlePair.Key;
		const FSoftObjectPath& Path = HandlePair.Value;
		
		// Find the cached data for this asset
		if (const FRecallAssetManagerLoadData* CachedData = AssetManagerData.AssetCache.Find(Path))
		{
			// Add to local cache
			RequestMap.Add(Handle, *CachedData);
		}
	}
}
