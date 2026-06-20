// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RecallSnapshotTypes.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h" // ERecallSnapshotReason

#include "RecallSnapshotSubsystem.generated.h"

/*
* Manage snapshots for our world.
* For simulation snapshots, take a look at MultiSimSubsystem.
*/
UCLASS()
class RECALLSNAPSHOT_API URecallSnapshotSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
		
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLoadSnapshot, uint32 /*Frame*/, double /*TimeSeconds*/);

	URecallSnapshotSubsystem();

public:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

	void TakeSnapshot(FRecallSimulationSnapshot& OutSnapshot, const FName& Reason = Recall::SnapshotReason::Snapshot) const;
	
	/** Load a snapshot, if not done on the game-thread, then pre-load and finish load snapshot separately */
	void LoadSnapshot(const FRecallSimulationSnapshot& Snapshot, const FName& Reason = Recall::SnapshotReason::Snapshot, bool bGameThread = true) const;

	/* Should be called on game-thread if LoadSnapshot was called async */
	void PreLoadSnapshot(const FRecallSimulationSnapshot& Snapshot, const FName& Reason = Recall::SnapshotReason::Snapshot) const;
	void FinishLoadSnapshot(const FRecallSimulationSnapshot& Snapshot, const FName& Reason = Recall::SnapshotReason::Snapshot) const;

	FOnLoadSnapshot OnLoadSnapshot;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallSimulationSubsystem> StepSimulationSystem;

};
