// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Helper/RecallGamethreadQueue.h"

#include "RecallAssetManagerTypes.generated.h"

#define RECALL_ASSET_LOAD_HANDLE_INVALID 0
#define RECALL_ASSET_LOAD_DEFAULT_DURATION 15

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallAsset, Log, All);

USTRUCT()
struct RECALLSIMULATION_API FRecallAssetLoadHandle
{
	GENERATED_BODY()

	FRecallAssetLoadHandle() = default;
	FRecallAssetLoadHandle(uint32 InSerialNumber) : SerialNumber(InSerialNumber) {}

	UPROPERTY(VisibleAnywhere)
	uint32 SerialNumber = RECALL_ASSET_LOAD_HANDLE_INVALID;

	bool IsValid() const;
	void Invalidate();

	friend uint32 GetTypeHash(const FRecallAssetLoadHandle& Handle)
	{
		return Handle.SerialNumber;
	}

	bool operator!=(const FRecallAssetLoadHandle& Other) const { return !(*this == Other); }
	bool operator==(const FRecallAssetLoadHandle& Other) const {	return SerialNumber == Other.SerialNumber; }
};

USTRUCT()
struct RECALLSIMULATION_API FRecallAssetManagerLoadData : public FRecallGamethreadRunnerData
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere)
	FSoftObjectPath AssetPath;

	UPROPERTY(VisibleAnywhere)
	int32 ReferenceCount = 0;

	bool operator==(const FRecallAssetManagerLoadData& Other) const
	{
		return AsyncStartFrame == Other.AsyncStartFrame
			&& AsyncEndFrame == Other.AsyncEndFrame
			&& AssetPath == Other.AssetPath;
	}

	bool operator!=(const FRecallAssetManagerLoadData& Other) const
	{
		return !operator==(Other);
	}
};

USTRUCT()
struct FRecallAssetManagerData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint32 SerialNumberGenerator = 0;

	UPROPERTY(VisibleAnywhere)
	TMap<FSoftObjectPath, FRecallAssetManagerLoadData> AssetCache;

	UPROPERTY(VisibleAnywhere)
	TMap<FRecallAssetLoadHandle, FSoftObjectPath> HandleToAssetPath;

	void Reset();
};

UCLASS(Within=RecallAssetManagerGamethreadQueue)
class URecallAssetManagerRunnerTask : public URecallGamethreadRunnerTask
{
	GENERATED_BODY()

public:
	UObject* GetLoadedAsset() const;

protected:
	virtual void OnRun() override final;
	virtual void OnForceStop() override final;
	virtual void OnForceFinish() override final;

	virtual bool IsIdenticalData(const TSharedPtr<FRecallGamethreadRunnerData>& OtherData) const override final
	{
		const TSharedPtr<FRecallAssetManagerLoadData> OtherAssetManagerLoadData = StaticCastSharedPtr<FRecallAssetManagerLoadData>(OtherData);
		check(OtherAssetManagerLoadData.IsValid());
		return GetRunnerDataRef<FRecallAssetManagerLoadData>() == (*OtherAssetManagerLoadData.Get());
	}
	
protected:
	UPROPERTY()
	TObjectPtr<UObject> LoadedAsset;
	
	TSharedPtr<struct FStreamableHandle> StreamableHandle;
	
	void OnFinished();
};

/**
* Manager our assets on the gamethread
*/
UCLASS(Within=RecallAssetManagerSubsystem)
class URecallAssetManagerGamethreadQueue : public URecallGamethreadQueue
{
	GENERATED_BODY()

public:
	URecallAssetManagerGamethreadQueue();

	void UpdateAssetManagerRunners(const TMap<FSoftObjectPath, FRecallAssetManagerLoadData>& AssetDataMap);
	UObject* GetLoadedAsset(const FSoftObjectPath& AssetPath) const;

private:
	TMap<FSoftObjectPath, uint32> AssetPathToHandle;
	TMap<uint32, FSoftObjectPath> HandleToAssetPath;
	uint32 NextHandle = 1;

	uint32 GetOrCreateHandle(const FSoftObjectPath& AssetPath);
	TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>> CreateDataMap(const TMap<FSoftObjectPath, FRecallAssetManagerLoadData>& AssetDataMap);
};
