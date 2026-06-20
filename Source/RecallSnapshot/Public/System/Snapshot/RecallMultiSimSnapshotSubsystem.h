// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Snapshot/RecallSnapshotInterface.h"
#include "RecallMultiSimSnapshotTypes.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h" // ERecallSnapshotReason

#include "RecallMultiSimSnapshotSubsystem.generated.h"

/*
* Snapshot system that handles having more than one world in parallel.
*/
UCLASS()
class RECALLSNAPSHOT_API URecallMultiSimSnapshotSubsystem :
	public UWorldSubsystem,
	public IRecallSnapshotInterface
{
	GENERATED_BODY()

public:
	URecallMultiSimSnapshotSubsystem();

	// UWorldSubsystem implementation Begin
public:
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSnapshotInterface implementation Begin
public:
	void SaveQuick() override final;
	void LoadQuick() override final;

	bool TakeSnapshot(TArray<uint8>& OutSnapshot) const override final;
	bool LoadSnapshot(const TArray<uint8>& Snapshot) override final;

	FOnLoadSnapshot& GetOnLoadSnapshotEvent() override final { return OnLoadSnapshot; }
	// IRecallSnapshotInterface implementation End

public:
	bool TakeQuickSnapshot(FRecallMultiSimSnapshot& OutSnapshot, const FName& Reason = Recall::SnapshotReason::Snapshot) const;
	bool LoadQuickSnapshot(const FRecallMultiSimSnapshot& Snapshot, const FName& Reason = Recall::SnapshotReason::Snapshot);

	static void SerializeSnapshot(const FRecallMultiSimSnapshot& Snapshot, TArray<uint8>& OutMemory);

	TArray<const UWorld*> GetMultiWorlds() const;
	
private:
#ifdef WITH_MULTI_WORLD
	TWeakObjectPtr<class UMultiWorldSubsystem> MultiWorldSystem;
#endif // WITH_MULTI_WORLD
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallMultiSimSubsystem> MultiSimSystem;

	FOnLoadSnapshot OnLoadSnapshot;
	TArray<uint8> QuickSnapshotMemory;
};
