// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Asset/RecallAssetManagerTypes.h"

#include "Engine/AssetManager.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

DEFINE_LOG_CATEGORY(LogRecallAsset);

bool FRecallAssetLoadHandle::IsValid() const
{
	return SerialNumber != RECALL_ASSET_LOAD_HANDLE_INVALID;
}

void FRecallAssetLoadHandle::Invalidate()
{
	SerialNumber = RECALL_ASSET_LOAD_HANDLE_INVALID;
}

void FRecallAssetManagerData::Reset()
{
	SerialNumberGenerator = 0;
	AssetCache.Reset();
	HandleToAssetPath.Reset();
}

//----------------------------------------------------------------------//
// URecallAssetManagerRunnerTask
//----------------------------------------------------------------------//
void URecallAssetManagerRunnerTask::OnRun()
{
	const FRecallAssetManagerLoadData& AssetManagerRunnerData = GetRunnerDataRef<FRecallAssetManagerLoadData>();

	FStreamableDelegate StreamableDelegate;
	StreamableDelegate.BindUObject(this, &ThisClass::OnFinished);

	StreamableHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(AssetManagerRunnerData.AssetPath, StreamableDelegate);
}

void URecallAssetManagerRunnerTask::OnFinished()
{
	if (IsFinished())
	{
		return;
	}

	LoadedAsset = StreamableHandle->GetLoadedAsset();
	
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);
	const FRecallAssetManagerLoadData& AssetManagerRunnerData = GetRunnerDataRef<FRecallAssetManagerLoadData>();
	UE_LOG(LogRecallAsset, Verbose, TEXT("Asset was loaded after %d frames: %s"),
		Frame - AssetManagerRunnerData.AsyncStartFrame, *AssetManagerRunnerData.AssetPath.ToString());
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallAssetManagerRunnerTask::OnForceStop()
{
	if (StreamableHandle.IsValid())
	{
		StreamableHandle->CancelHandle();
		StreamableHandle.Reset();
	}
}

void URecallAssetManagerRunnerTask::OnForceFinish()
{
	const FRecallAssetManagerLoadData& AssetManagerRunnerData = GetRunnerDataRef<FRecallAssetManagerLoadData>();
	LoadedAsset = AssetManagerRunnerData.AssetPath.TryLoad();
}

UObject* URecallAssetManagerRunnerTask::GetLoadedAsset() const
{
	return LoadedAsset;
}

//----------------------------------------------------------------------//
// URecallAssetManagerGamethreadQueue
//----------------------------------------------------------------------//
URecallAssetManagerGamethreadQueue::URecallAssetManagerGamethreadQueue()
	: Super()
{
	RunnerTaskClass =  URecallAssetManagerRunnerTask::StaticClass();
}

void URecallAssetManagerGamethreadQueue::UpdateAssetManagerRunners(const TMap<FSoftObjectPath, FRecallAssetManagerLoadData>& AssetDataMap)
{
	return UpdateRunners(CreateDataMap(AssetDataMap));
}

uint32 URecallAssetManagerGamethreadQueue::GetOrCreateHandle(const FSoftObjectPath& AssetPath)
{
	if (uint32* Existing = AssetPathToHandle.Find(AssetPath))
	{
		return *Existing;
	}
	
	const uint32 NewHandle = NextHandle++;
	AssetPathToHandle.Add(AssetPath, NewHandle);
	
	return NewHandle;
}

TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>> URecallAssetManagerGamethreadQueue::CreateDataMap(
	const TMap<FSoftObjectPath, FRecallAssetManagerLoadData>& AssetDataMap)
{
	TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>> DataMap;
	DataMap.Reserve(AssetDataMap.Num());

	for (const TPair<FSoftObjectPath, FRecallAssetManagerLoadData>& AssetManagerData : AssetDataMap)
	{
		const uint32 Handle = GetOrCreateHandle(AssetManagerData.Key);
		
		// Check if we already have a version of this asset
		if (TSharedPtr<FRecallGamethreadRunnerData>* ExistingData = DataMap.Find(Handle))
		{
			// Only replace if the new version is more recent (higher AsyncStartFrame)
			const uint32 ExistingEndFrame = ExistingData->Get()->AsyncEndFrame;
			const uint32 NewEndFrame = AssetManagerData.Value.AsyncEndFrame;
			
			if (NewEndFrame < ExistingEndFrame)
			{
				DataMap.Add(Handle, MakeShared<FRecallAssetManagerLoadData>(AssetManagerData.Value));
			}
		}
		else
		{
			// First time we see this asset, add it
			DataMap.Add(Handle, MakeShared<FRecallAssetManagerLoadData>(AssetManagerData.Value));
		}
	}

	return DataMap;
}

UObject* URecallAssetManagerGamethreadQueue::GetLoadedAsset(const FSoftObjectPath& AssetPath) const
{
	const uint32* Handle = AssetPathToHandle.Find(AssetPath);
	if (!ensureMsgf(Handle,
		TEXT("%hs No handle for: %s"), __FUNCTION__, *AssetPath.ToString()))
	{
		return nullptr;
	}
	
	const URecallAssetManagerRunnerTask* RunnerTask = GetRunnerTask<URecallAssetManagerRunnerTask>(*Handle);
	if (!ensureAlwaysMsgf(RunnerTask,
		TEXT("%hs No task"), __FUNCTION__))
	{
		return nullptr;
	}

	if (!ensureAlwaysMsgf(RunnerTask->IsFinished(),
		TEXT("%hs Failed to finish AssetLoad"), __FUNCTION__))
	{
		return nullptr;
	}

	return RunnerTask->GetLoadedAsset();
}
