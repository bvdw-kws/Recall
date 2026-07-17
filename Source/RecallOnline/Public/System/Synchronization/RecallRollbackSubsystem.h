// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "System/Synchronization/RecallSynchronizationInterface.h"
#include "RecallRollbackTypes.h"
#include "System/Synchronization/RecallRollbackFrameManager.h"
#include "Utility/Rollback/RecallDebugRollbackManager.h"
#include "Utility/Rollback/RecallRollbackConfig.h"

#include "RecallRollbackSubsystem.generated.h"

/*
* Handle rollback and synchronization of our simulation between players.
* Only the main world subsystem is used.
*/
UCLASS()
class RECALLONLINE_API URecallRollbackSubsystem :
	public UWorldSubsystem, 
	public IRecallSimulationReactSystemInterface,
	public IRecallSynchronizationInterface
{
	GENERATED_BODY()

	URecallRollbackSubsystem();

public:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSynchronizationInterface implementation Begin
	virtual bool IsRollbackEnabled() const override final;
	virtual uint32 GetLastSyncedFrame() const override final;
	virtual int32 GetRollbackFrameCount() const override final;
	virtual bool ShouldNetPause() const override final;

	virtual void ForceLastSyncedFrame(uint32 Frame) override final;
	virtual void ResetSyncedFrame(uint32 Frame) override final;

	virtual void SetPauseRollback(bool bPause) override final;
	virtual bool IsDuringRollback() const override final { return SyncingSimulationUntil != 0; }

	virtual void SetConfirmFrame(uint32 InConfirmFrame) override final;
	virtual uint32 GetConfirmFrame() const override final;

	virtual bool SaveLastSyncedSnapshot(TArray<uint8>& OutSnapshotMemory) const override final;
	virtual void LoadLastSyncedSnapshot(const TArray<uint8>& SnapshotMemory) const override final;
	// IRecallSynchronizationInterface implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

	DECLARE_MULTICAST_DELEGATE(FRollbackEvent);
	FRollbackEvent OnRollbackEnd;

protected:
	// Last synced frame for confirm frame
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 LastSyncedFrame{ 0 };

	UPROPERTY(VisibleAnywhere, Transient)
	bool bPauseRollback{ false };

private:
	// Configuration helper
	FRecallRollbackConfig Config;

	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallSynchronizationContainerSubsystem> ContainerSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallMultiSimSubsystem> MultiSimSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallMultiSimSnapshotSubsystem> MultiSimSnapshotSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallSimulationInsightSubsystem> SimInsightSystem;

	UPROPERTY(Transient)
	uint32 SyncingSimulationUntil = 0;

	// Cache our confirm frame in case it changes during rollback
	UPROPERTY(Transient)
	uint32 RollbackConfirmFrame = 0;

	FRecallRollbackFrameManager FrameManager;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	FRecallDebugRollbackManager DebugManager;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	void OnFrameSync(uint32 Frame);

	// Frame Management
	bool ShouldSaveRollbackSnapshot(uint32 Frame) const;
	void SaveFrame(uint32 Frame);

	// Sync Processing
	bool SyncFrame(uint32 Frame);
	
	// Frame Manager Context Creation
	FRecallRollbackFrameContext CreateFrameManagerContext();
	
	// Core Utilities
	bool IsPauseRollback() const;
	void SetSyncingSimulationUntil_Internal(uint32 Frame);

	// Debug & Configuration
	void HandleRollbackEnd(uint32 Frame);
	void HandleDebugRollbackComparison(uint32 Frame);

	// Forces LastSyncedFrame to the newest valid snapshot <= RollbackConfirmFrame + 1.
	// The replay just recomputed everything up to the confirmed frame using
	// authoritative inputs, so that state is correct even if it never gets
	// naturally classified as "synced" (e.g. desyncs recurring every replay).
	void ForceSyncToConfirmedFrame(uint32 Frame);
	
	// Configuration Management
	void BeginRollbackState(uint32 Frame);
	void EndRollbackState(uint32 CurrentFrame);
	void UpdateConfirmFrameState(uint32 Frame);

};
