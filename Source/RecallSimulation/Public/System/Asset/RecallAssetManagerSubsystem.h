// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "RecallAssetManagerTypes.h"

#include "RecallAssetManagerSubsystem.generated.h"

UCLASS()
class RECALLSIMULATION_API URecallAssetManagerSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	URecallAssetManagerSubsystem();

	// UWorldSubsystem implementation Begin
public:
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
public:
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

public:
	FRecallAssetLoadHandle RequestAsset(const FSoftObjectPath& Path, int32 LoadDuration = RECALL_ASSET_LOAD_DEFAULT_DURATION);
	void ReleaseAsset(FRecallAssetLoadHandle& Handle);

	bool IsAssetLoaded(const FRecallAssetLoadHandle& Handle) const;
	UObject* GetLoadedAsset(const FRecallAssetLoadHandle& Handle) const;

	template<typename T>
	T* GetLoadedAsset(const FRecallAssetLoadHandle& Handle) const
	{
		return CastChecked<T>(GetLoadedAsset(Handle));
	}

	/**
	 * Clear the asset cache. This will force all assets to be reloaded on next request.
	 */
	void ClearAssetCache();

protected:
	UPROPERTY(VisibleAnywhere)
	FRecallAssetManagerData AssetManagerData;

private:
	UPROPERTY(Transient)
	TObjectPtr<URecallAssetManagerGamethreadQueue> AssetQueue;
	
	// Local cache for active handles
	UPROPERTY(Transient)
	TMap<FRecallAssetLoadHandle, FRecallAssetManagerLoadData> RequestMap;
	
	mutable FCriticalSection DataGuard;

	void OnTickStart(float DeltaTime);
	void RegenerateRequestMap();
};

template<>
struct TMassExternalSubsystemTraits<URecallAssetManagerSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
