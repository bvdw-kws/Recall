// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Input/RecallInputQueueTypes.h"
#include "System/Observer/RecallObserverSubjectInterface.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"

#include "RecallMultiSimSubsystem.generated.h"

enum class ERecallSnapshotReason;

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallMultiSim, Log, All);

/*
* Manage our simulation with multiple worlds running in parallel.
* This system should only be running in our main world.
*/
UCLASS()
class RECALLSIMULATION_API URecallMultiSimSubsystem : 
	public UTickableWorldSubsystem, 
	public IRecallSimulationControllerInterface,
	public IRecallObserverSubjectInterface
{
	GENERATED_BODY()

	DECLARE_MULTICAST_DELEGATE(FOnSimulationEvent);

	URecallMultiSimSubsystem();

public:
	void Rollback_Step(int32 Step);
	void StepSimulation();

	bool ShouldRenderSimulation() const { return bRenderSimulation; }
	bool IsSimulationProcessingPhase() const { return bSimulationProcessingPhase.Load(); }
	bool IsRenderProcessingPhase() const { return bRenderProcessingPhase.Load(); }

	void WaitForStepThread() const;
	void OnLoadSnapshot(uint32 Frame, double TimeSeconds, bool bIsRollback);

	FOnFrameEvent& GetOnFrameSyncEvent() { return OnFrameSyncEvent; }
	FOnSimulationEvent& GetOnSimulationStart() { return OnSimulationStart; }
	FOnSimulationEvent& GetOnBeginSimulationProcessingPhase() { return OnBeginSimulationProcessingPhase; }
	FOnSimulationEvent& GetOnEndSimulationProcessingPhase() { return OnEndSimulationProcessingPhase; }

	// IRecallSimulationControllerInterface implementation Begin
public:
	virtual void StartSimulation(const FRecallSimulationStartParams& Params, bool bResume = true) override final;
	virtual void PauseSimulation() override final;
	virtual void ResumeSimulation() override final;
	virtual void ResetSimulation() override final;
	virtual void RestartSimulation(const FRecallSimulationStartParams& Params, bool bResume = true) override final;
	virtual bool IsSimulationPaused() const override final;
	virtual bool HasSimulationStarted() const override final { return bStarted; }

	virtual double GetElapsedTime() const override final { return ElapsedTime.Load(); }
	virtual uint32 GetElapsedFrame() const override final { return ElapsedFrame.Load(); };

	virtual void SetSpeedScale(float InSpeedScale) override final;
	virtual void SetRenderSimulation(bool bInRenderSimulation) override final { bRenderSimulation = bInRenderSimulation; }
	virtual void SetForceStepSim(uint32 Step = 1) override final;

	virtual void PushInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input) override final;
	virtual bool TryPushLocalInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input) override final;

	virtual void RequestAddPlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams) override final;
	virtual void RequestRemovePlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId) override final;

	virtual void SetFramesPerSecond(int32 FramesPerSecond) override final;

	virtual FOnTickEvent& GetOnTickStartEvent() override final { return OnTickStartEvent; }
	virtual FOnTickEvent& GetOnTickEndEvent() override final { return OnTickEndEvent; }
	virtual FOnFrameEvent& GetOnFrameStartEvent() override final { return OnFrameStartEvent; }
	virtual FOnInputPhaseEvent& GetOnInputPhaseEvent() override final { return OnInputPhaseEvent; }
	virtual FOnFrameEvent& GetOnLoadSnapshotEvent() override final { return OnLoadSnapshotEvent; }
	// virtual FOnWorldTransitionEvent& GetOnWorldTransitionEvent() override { return OnWorldTransitionEvent; }
	// IRecallSimulationControllerInterface implementation End

public:
	// FTickableGameObject implementation Begin
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject implementation End

	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallObserverSubjectInterface implementation Begin
	virtual void RegisterObserver(UClass* Class, UObject* ObjectPointer) override final;
	virtual void UnregisterObserver(UObject* SourceObject) override final;
	// IRecallObserverSubjectInterface implementation End

protected:
	UPROPERTY(VisibleAnywhere, Transient)
	float SpeedScale{ 1.0f };
	
	UPROPERTY(VisibleAnywhere, Transient)
	float AccumulatedTime{ 0.0 };

	// Written from the multi-sim step thread and read from the game thread (e.g. input capture,
	// rollback confirm-frame bookkeeping), so these need to be atomic rather than plain UPROPERTY
	// fields (UHT doesn't support TAtomic members, hence dropping UPROPERTY here).
	TAtomic<double> ElapsedTime{ 0.0 };

	TAtomic<uint32> ElapsedFrame{ 0 };

	UPROPERTY(VisibleAnywhere, Transient)
	float DeltaFrames = 0.0f;

	UPROPERTY(VisibleAnywhere, Transient)
	bool bStarted{ false };

	UPROPERTY(VisibleAnywhere, Transient)
	bool bPause{ true };

	// UPROPERTY(VisibleAnywhere, Transient)
	// uint32 ForceStepSim{ 0 };

	UPROPERTY(VisibleAnywhere, Transient)
	bool bRenderSimulation{ true };

	float SmoothedDeltaTime = 0.01667f;
	float PrevDeltaTime = 0.01667f;
	float DeltaTimeDecay = 0.1f;

private:
	// Written from the multi-sim step thread and read from the game thread, so these need to be
	// atomic rather than plain UPROPERTY fields (UHT doesn't support TAtomic members, hence dropping
	// UPROPERTY here).
	TAtomic<bool> bSimulationProcessingPhase{ false };

	TAtomic<bool> bRenderProcessingPhase{ false };

	UPROPERTY(Transient)
	uint32 StepCount = 0;

	TSharedPtr<class FRecallMultiSimStepThread> StepThread;
	FCriticalSection SimulationGuard;
	FCriticalSection DeltaGuard;

	// FOnWorldTransitionEvent OnWorldTransitionEvent;

	FOnSimulationEvent OnSimulationStart;
	FOnTickEvent OnTickStartEvent;
	FOnTickEvent OnTickEndEvent;
	FOnFrameEvent OnFrameSyncEvent;
	FOnFrameEvent OnFrameStartEvent;
	FOnFrameEvent OnLoadSnapshotEvent;
	FOnInputPhaseEvent OnInputPhaseEvent;

	FOnSimulationEvent OnBeginSimulationProcessingPhase;
	FOnSimulationEvent OnEndSimulationProcessingPhase;

	void StartTick(float DeltaTime);
	void EndTick(float DeltaTime);
	bool HandleInputPhase(uint32 Frame);
	void RenderSimulation(float DeltaTime, bool bResetRender = false);

	float GetSimulationRate() const;
	uint32 GetMaxStepCount() const;

	void OnWorldBeginTearDown(UWorld* World);
};
