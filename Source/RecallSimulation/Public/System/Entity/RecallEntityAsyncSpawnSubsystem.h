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
#include "Mass/ExternalSubsystemTraits.h"
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
	
	const FRecallEntityAsyncSpawnContext* PeekSpawnContext() const;
	
public:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Start(const FRecallSimulationStartParams& Params) override final;
	virtual int32 GetStartOrderPriority() const override final { return Recall::SimReactSystem::StartOrder::MediumPriority; }
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

private:
	friend class URecallEntityAsyncSpawnProcessor;
	
	bool IsAnyRequestReady() const;
	void PushQueue(FMassEntityManager& System);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallAssetManagerSubsystem> AssetManagerSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallEntitySubsystem> EntitySystem;

	mutable FCriticalSection DataGuard;
	TSharedPtr<struct FRecallEntityAsyncSpawnQueue> SpawnQueue;

	/* Spawn context for the request currently being created, valid only while SpawnRequestEntity is creating its entities. */
	mutable TUniquePtr<FRecallEntityAsyncSpawnContext> SpawnContext;
	
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
