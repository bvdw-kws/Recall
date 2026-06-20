// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "System/Player/RecallPlayerQueueInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "System/Player/RecallPlayerQueueTypes.h"

#include "RecallPlayerQueueSubsystem.generated.h"

/*
* Keep track of our player queue data so it can be saved/restored
*/
USTRUCT()
struct FRecallPlayerQueueState
{
	GENERATED_BODY()

	// Queue of player events
	UPROPERTY(VisibleAnywhere)
	FRecallPlayerQueue EventQueue;

	// Next player (in the queue) to be spawned
	UPROPERTY(VisibleAnywhere)
	int32 CurrentItemIndex{ 0 };
};

struct FRecallPlayerEvent;

/*
* Manage requests - from outside the simulation - to add or remove players.
*/
UCLASS()
class RECALLSIMULATION_API URecallPlayerQueueSubsystem : 
	public UWorldSubsystem, 
	public IRecallPlayerQueueInterface,
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	bool PopNextPlayerEvent(FRecallPlayerEvent& OutEvent);
	bool IsStartField(const FString& PlayerId) const;

	void ApplyFrameCommitToPlayerArray(TArray<FString>& PlayerIds, bool bLocalPlayer = false) const;
	void RollbackFrameForPlayerArray(uint32 Frame, TArray<FString>& PlayerIds) const;

protected:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

	// IRecallSimulationReactSystemInterface implementation Begin
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

	// IRecallPlayerQueueInterface implementation Begin
	void RequestAddPlayer(uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams) override final;
	void RequestRemovePlayer(uint32 Frame, const FString& PlayerId) override final;

	const FRecallPlayerQueue& GetPlayerQueue() const override final { return PlayerQueue.EventQueue; }
	void SetPlayerQueue(const FRecallPlayerQueue& InQueue) override final { PlayerQueue.EventQueue = InQueue; }

	void ClearPlayerQueuePastFrame(uint32 Frame) override final;
	// IRecallPlayerQueueInterface implementation End

protected:
	UPROPERTY(VisibleAnywhere, Transient)
	FRecallPlayerQueueState PlayerQueue;

private:
	mutable FCriticalSection DataGuard;

	void PushPlayerRequest(const FRecallPlayerQueueItem& Item);

};

template<>
struct TMassExternalSubsystemTraits<URecallPlayerQueueSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
