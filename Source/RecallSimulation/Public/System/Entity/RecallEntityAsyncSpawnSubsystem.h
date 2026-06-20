// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassEntityManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "RecallEntityAsyncSpawnTypes.h"

#include "RecallEntityAsyncSpawnSubsystem.generated.h"

struct FRecallEntityAsyncSpawnRequest;
class UMassEntityConfigAsset;

/**
* System to spawn entities asynchronously.
*/
UCLASS()
class RECALLSIMULATION_API URecallEntityAsyncSpawnSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	URecallEntityAsyncSpawnSubsystem();

public:
	void SpawnEntityAsync(const TSoftObjectPtr<UMassEntityConfigAsset>& EntityConfigAsset,
		const FVector& Position, FQuat Rotation = FQuat::Identity,
		TOptional<FRecallEntityAsyncSpawnParameters> Params = TOptional<FRecallEntityAsyncSpawnParameters>());
	void SpawnEntityAsync(const FSoftObjectPath& EntityConfigAsset,
		const FVector& Position, FQuat Rotation = FQuat::Identity,
		TOptional<FRecallEntityAsyncSpawnParameters> Params = TOptional<FRecallEntityAsyncSpawnParameters>());
	
	const FRecallEntityAsyncSpawnContext* GetSpawnContext() const;
	
private:
	friend class URecallEntityAsyncSpawnProcessor;
	
	bool IsAnyRequestReady() const;
	void PushQueue(FMassEntityManager& System);
	
protected:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	void Start(const FRecallSimulationStartParams& Params) override final;
	int32 GetStartOrderPriority() const override final { return Recall::SimReactSystem::StartOrder::MediumPriority; }
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallAssetManagerSubsystem> AssetManagerSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallEntitySubsystem> EntitySystem;

	TSharedPtr<struct FRecallEntityAsyncSpawnQueue> SpawnQueue;
	mutable TUniquePtr<FRecallEntityAsyncSpawnContext> SpawnContext;

	mutable FCriticalSection DataGuard;
	
	bool SpawnRequestEntity(FMassEntityManager& System, const FRecallEntityAsyncSpawnRequest& Request) const;
	FRecallEntityAsyncSpawnQueue& GetMutableSpawnQueue();
	const FRecallEntityAsyncSpawnQueue& GetSpawnQueue() const;
};

template<>
struct TMassExternalSubsystemTraits<URecallEntityAsyncSpawnSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
