// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Asset/RecallAssetManagerSubsystem.h"

#include "Desync/RecallDesyncLog.h"
#include "Engine/World.h"
#include "Subsystems/SubsystemCollection.h"
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
	checkf(LoadDuration > 0, TEXT("Request load duration must be positive"));
	
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
			AssetManagerData.HandleToAssetPath.Add(Handle, Path);			
			
			UE_LOG(LogRecallAsset, Verbose,
				TEXT("%hs Asset: %s, RefCount=%d"), __FUNCTION__, *Path.ToString(), CachedData->ReferenceCount);
		}
		else
		{
			const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);
			const int32 MaxStepCount = Recall::Simulation::Utils::GetMaxStepCount(this);
		
			// Asset not cached, create new load data
			FRecallAssetManagerLoadData& Request = AssetManagerData.AssetCache.Add(Path);
			Request.AsyncStartFrame = Frame;
			Request.AsyncEndFrame = Frame + MaxStepCount + LoadDuration;
			Request.AssetPath = Path;
			Request.ReferenceCount = 1;
			
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
		FSoftObjectPath AssetPath;
		if (AssetManagerData.HandleToAssetPath.RemoveAndCopyValue(Handle, AssetPath))
		{
			// Decrement reference count in cache
			FRecallAssetManagerLoadData& CachedData = AssetManagerData.AssetCache.FindChecked(AssetPath);			
			CachedData.ReferenceCount--;
				
			// Remove from cache if no more references
			if (CachedData.ReferenceCount <= 0)
			{
				AssetManagerData.AssetCache.Remove(AssetPath);
					
				UE_LOG(LogRecallAsset, Verbose,
					TEXT("%hs Asset: %s, RefCount=0 (removed)"), __FUNCTION__, *AssetPath.ToString());
			}
			else
			{					
				UE_LOG(LogRecallAsset, Verbose,
					TEXT("%hs Asset: %s, RefCount=%d"), __FUNCTION__,
					*AssetPath.ToString(), CachedData.ReferenceCount);
			}
		
			// Remove from handle mapping
			AssetManagerData.HandleToAssetPath.Remove(Handle);
		}
	}
	
	Handle.Invalidate();
}

bool URecallAssetManagerSubsystem::IsAssetLoaded(const FRecallAssetLoadHandle& Handle) const
{
	if (!Handle.IsValid())
	{
#if RECALL_DESYNC_LOG
		RECALL_DESYNC_LOG_STR(this, IsAssetLoaded,
			FString::Printf(TEXT("InvalidHandle (Result: false)")));
#endif // RECALL_DESYNC_LOG
		return false;
	}

	const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);

	FScopeLock Lock(&DataGuard);
	const FSoftObjectPath* SoftObjectPtr = AssetManagerData.HandleToAssetPath.Find(Handle);
	if (ensureAlwaysMsgf(SoftObjectPtr != nullptr,
		TEXT("%hs Asset handle was not found"), __FUNCTION__))
	{
		const FRecallAssetManagerLoadData* RequestPtr = AssetManagerData.AssetCache.Find(*SoftObjectPtr);
		if (ensureMsgf(RequestPtr != nullptr,
			TEXT("%hs Asset was not requested"), __FUNCTION__))
		{
			const bool bResult = Frame >= RequestPtr->AsyncEndFrame;
#if RECALL_DESYNC_LOG
			RECALL_DESYNC_LOG_STR(this, IsAssetLoaded,
				FString::Printf(TEXT("%s (Handle: %u, Frame: %d, AsyncStartFrame: %d, AsyncEndFrame: %d, Result: %s)"),
					*SoftObjectPtr->ToString(), Handle.SerialNumber, Frame,
					RequestPtr->AsyncStartFrame, RequestPtr->AsyncEndFrame,
					*Recall::Desync::Conv_BoolToString(bResult)));
#endif // RECALL_DESYNC_LOG
			return bResult;
		}
	}
#if RECALL_DESYNC_LOG
	RECALL_DESYNC_LOG_STR(this, IsAssetLoaded,
		FString::Printf(TEXT("NotFound (Result: false)")));
#endif // RECALL_DESYNC_LOG
	return false;
}

UObject* URecallAssetManagerSubsystem::GetLoadedAsset(const FRecallAssetLoadHandle& Handle) const
{
	if (ensure(Handle.IsValid()) && AssetQueue)
	{
		// Find the asset path for this handle
		if (const FSoftObjectPath* AssetPathPtr = AssetManagerData.HandleToAssetPath.Find(Handle))
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
}
