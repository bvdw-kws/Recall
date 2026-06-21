// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Simulation/RecallMultiSimSubsystem.h"

#include "Engine/World.h"
#include "MassProcessingTypes.h"
#include "System/Input/RecallInputQueueSubsystem.h"
#include "System/Observer/RecallObserverSubjectSubsystem.h"
#include "System/Player/RecallPlayerQueueSubsystem.h"
#include "System/Simulation/RecallSimulationSubsystem.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#include "Thread/RecallMultiSimStepThread.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Player/RecallPlayerUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

DEFINE_LOG_CATEGORY(LogRecallMultiSim);

static bool bSmoothenGameSimDeltaTime = false;
static FAutoConsoleVariableRef CVarRecallSmoothenGameSimDeltaTime(
	TEXT("recall.simulation.SmoothenDeltaTime"),
	bSmoothenGameSimDeltaTime,
	TEXT("Smoothen Game Sim Delta Time")
);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
static bool bDebugPauseSimulation = false;
static FAutoConsoleCommand CVarRecallDebugPauseSimulation(
	TEXT("recall.simulation.Pause"),
	TEXT("Pause the simulation"),
	FConsoleCommandDelegate::CreateLambda(
		[]()
		{
			bDebugPauseSimulation = !bDebugPauseSimulation;
		}
	)
);

static float DebugSimulationSpeed = 1.0f;
static FAutoConsoleVariableRef CVarRecallDebugSimulationSpeed(
	TEXT("recall.simulation.Speed"),
	DebugSimulationSpeed,
	TEXT("Control the speed of the simulation")
);

static bool bDebugRenderSimulation = true;
static FAutoConsoleVariableRef CVarRecallDebugRenderSimulation(
	TEXT("recall.simulation.Render"),
	bDebugRenderSimulation,
	TEXT("Render the simulation")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

URecallMultiSimSubsystem::URecallMultiSimSubsystem()
	: Super()
{
}

void URecallMultiSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Recall::MultiWorld::Utils::InitializeMultiWorldDependency(Collection);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	// Reset pause flag when enter a new level.
	bDebugPauseSimulation = false;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::OnWorldBeginTearDown);
}

void URecallMultiSimSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ensureAlwaysMsgf(!StepThread.IsValid(),
		TEXT("%hs Step thread must stopped before the world is teared down"), __FUNCTION__);

	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
}

void URecallMultiSimSubsystem::OnWorldBeginTearDown(UWorld* World)
{
	if (World && World == GetWorld() && StepThread.IsValid())
	{
		StepThread->Exit();
		StepThread->Stop();
		StepThread.Reset();
	}
}

void URecallMultiSimSubsystem::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::Tick"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_Tick);

	const UWorld* World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld() || !World->HasBegunPlay())
	{
		return;
	}

	if (!HasSimulationStarted())
	{
		return;
	}

	if (StepThread.IsValid() && !StepThread->IsStepDone())
	{
		UE_LOG(LogRecallMultiSim, VeryVerbose,
			TEXT("%hs Game-thread is behind simulation step thread"), __FUNCTION__)
		
		StepThread->WaitForStep();
	}

	checkf(!IsRollback(this),
		TEXT("%hs Rollback should only happens while simulation is stepping"), __FUNCTION__);
	
	StartTick(DeltaTime);

	RenderSimulation(DeltaTime);
	
	// To count the number of frames that occured for this tick we reset the frame count.
	{
		FScopeLock DeltaLock(&DeltaGuard);
		DeltaFrames = 0.0f;
	}

	if (IsSimulationPaused() == false && IsNetPause(this) == false)
	{
		SmoothedDeltaTime = PrevDeltaTime + (DeltaTime - PrevDeltaTime) * DeltaTimeDecay;
		PrevDeltaTime = DeltaTime;

		const float StepDeltaTime = bSmoothenGameSimDeltaTime ? SmoothedDeltaTime : DeltaTime;
		const float SimulationRate = GetSimulationRate();
		AccumulatedTime += static_cast<double>(FMath::Max(0.0f, StepDeltaTime * SimulationRate));

		const int32 FramesPerSecond = Recall::Simulation::Utils::GetFramesPerSeconds(this);
		const uint32 AccumulatedFrameCount = static_cast<uint32>(AccumulatedTime * static_cast<double>(FramesPerSecond));
			
		if (AccumulatedFrameCount > 0)
		{
			StepCount = FMath::Min(GetMaxStepCount(), AccumulatedFrameCount);
			AccumulatedTime -= static_cast<float>(StepCount) / static_cast<float>(FramesPerSecond);
		}
	}

	if (StepCount > 0 && StepThread.IsValid())
	{
		// Step frame events
		for (uint32 Step = 0; Step < StepCount; Step++)
		{
			const uint32 StepFrame = ElapsedFrame + Step;
			
			// Input phase
			{
				FScopeLock SimulationLock(&SimulationGuard);
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
				if (!ensure(HandleInputPhase(StepFrame)))
				{
					// Fallback so we can debug this error
					HandleInputPhase(StepFrame);
				}
#else // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
				HandleInputPhase(StepFrame);
#endif
			}
			
			OnFrameStartEvent.Broadcast(StepFrame);
		}
		
		StepThread->Simulation_Step(StepCount);
		StepCount = 0;
	}

	EndTick(DeltaTime);
}

TStatId URecallMultiSimSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URecallMultiSimSubsystem, STATGROUP_Tickables);
}

bool URecallMultiSimSubsystem::IsSimulationPaused() const
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (bDebugPauseSimulation)
	{
		return true;
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	return bPause; 
}

void URecallMultiSimSubsystem::SetSpeedScale(float InSpeedScale)
{
	checkf(InSpeedScale >= 0.0f, TEXT("Speed scale cannot be negative"));
	SpeedScale = FMath::Max(InSpeedScale, 0.0f);
}

float URecallMultiSimSubsystem::GetSimulationRate() const
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	return SpeedScale * DebugSimulationSpeed;
#else // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	return SpeedScale;
#endif
}

void URecallMultiSimSubsystem::StartSimulation(const FRecallSimulationStartParams& Params, bool bResume /*= true*/)
{
	FScopeLock SimulationLock(&SimulationGuard);

	if (!ensureMsgf(HasSimulationStarted() == false,
		TEXT("%hs Start should only be called once."), __FUNCTION__))
	{
		return;
	}

	bSimulationProcessingPhase = true;

	for (const UWorld* World : Recall::MultiWorld::Utils::GetMultiWorlds(this))
	{
		if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
		{
			SimulationSystem->Start(Params);
		}
	}

	bSimulationProcessingPhase = false;

	bStarted = true;

	if (bResume)
	{
		ResumeSimulation();
	}

	StepThread = MakeShared<FRecallMultiSimStepThread>(this);
	StepThread->StartTick();

	OnSimulationStart.Broadcast();
}

void URecallMultiSimSubsystem::ResetSimulation()
{
	if (StepThread.IsValid())
	{
		StepThread->Exit();
		StepThread->Stop();
		StepThread.Reset();
	}

	FScopeLock SimulationLock(&SimulationGuard);

	ElapsedTime = 0.0;
	ElapsedFrame = 0;
	AccumulatedTime = 0.0f;

	for (const UWorld* World : Recall::MultiWorld::Utils::GetMultiWorlds(this))
	{
		if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
		{
			SimulationSystem->Reset();
		}
	}

	// Restart our simulation.
	bStarted = false;
}

void URecallMultiSimSubsystem::RestartSimulation(const FRecallSimulationStartParams& Params, bool bResume /*= true*/)
{
	ResetSimulation();
	StartSimulation(Params, bResume);
}

bool URecallMultiSimSubsystem::HandleInputPhase(uint32 Frame)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::HandleInputPhase"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_HandleInputPhase);

	// Gather our player inputs before starting the frame.
	const TArray<FString> LocalPlayerIds = Recall::Player::Utils::FindLocalPlayerInputPhase(this, Frame);

	for (const FString& LocalPlayerId : LocalPlayerIds)
	{
		OnInputPhaseEvent.Broadcast(Frame, LocalPlayerId);
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	return ensureAlwaysMsgf(!Recall::Player::Utils::HasAnyLocalPlayerInputPhase(this, Frame),
		TEXT("[%s] could not play his input phase"), *LocalPlayerIds[0]);
#else // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	return true;
#endif
}

void URecallMultiSimSubsystem::StartTick(float DeltaTime)
{
	{
    	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::OnTickStartEvent"));
    	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_OnTickStartEvent);
    	OnTickStartEvent.Broadcast(DeltaTime);
    }

	const TArray<const UWorld*> WorldsForTick = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	if (!WorldsForTick.IsEmpty())
	{
    	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::StartTick"));
    	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_StartTick);

    	for (const UWorld* World : WorldsForTick)
    	{
    		if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
    		{
    			SimulationSystem->OnTickStart();
    		}
    	}
    }
}

void URecallMultiSimSubsystem::EndTick(float DeltaTime)
{
	{		
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::OnTickEndEvent"));
    	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_OnTickEndEvent);
		OnTickEndEvent.Broadcast(DeltaTime);
	}
}

void URecallMultiSimSubsystem::StepSimulation()
{
	FScopeLock SimulationLock(&SimulationGuard);

	OnFrameSyncEvent.Broadcast(GetElapsedFrame()); // Rollback

	OnBeginSimulationProcessingPhase.Broadcast();

	bSimulationProcessingPhase = true;

	// Count how many frame steps occurred per tick of the simulation.
	{
		FScopeLock DeltaLock(&DeltaGuard);
		DeltaFrames += 1.0f;
	}

	// Step our game simulation per world.
	const TArray<const UWorld*> WorldsToStep = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	if (!WorldsToStep.IsEmpty())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::Step"));
		QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_Step);

		for (const UWorld* World : WorldsToStep)
		{
			if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
			{
				ensureMsgf(GetElapsedFrame() == SimulationSystem->GetSimulationFrame(),
					TEXT("%hs Worlds should all be at the same frame."), __FUNCTION__);

				SimulationSystem->StartFrame();
			}
		}

		for (int32 PhaseIndex = 0; PhaseIndex < int32(EMassProcessingPhase::Render); PhaseIndex++)
		{
			const EMassProcessingPhase Phase = static_cast<EMassProcessingPhase>(PhaseIndex);

			for (const UWorld* World : WorldsToStep)
			{
				if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
				{
					SimulationSystem->Step(Phase);
				}
			}
		}

		for (const UWorld* World : WorldsToStep)
		{
			if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
			{
				SimulationSystem->EndFrame();
			}
		}
	}

	const uint32 CurrentFrame = GetElapsedFrame();
	ElapsedFrame++;

	// We only allow the simulation to update at the normal rate while frame stepping.
	const float FixedDeltaTime = Recall::Simulation::Utils::GetFixedDeltaTime(this);
	ElapsedTime += static_cast<double>(FixedDeltaTime);

	bSimulationProcessingPhase = false;

	OnEndSimulationProcessingPhase.Broadcast();
}

void URecallMultiSimSubsystem::SetForceStepSim(uint32 Step /*= 1*/)
{
	const int32 MaxStepCount = Recall::Simulation::Utils::GetMaxStepCount(this);	
	StepCount = FMath::Min(static_cast<uint32>(MaxStepCount), Step);
}

uint32 URecallMultiSimSubsystem::GetMaxStepCount() const
{
	const uint32 ConfirmFrame = GetConfirmFrame(this);
	const int32 RollbackFrameCount = GetRollbackFrameCount(this);	
	const int32 MaxRollbackFrame = ElapsedFrame <= ConfirmFrame ? 0 : static_cast<int32>(ElapsedFrame - ConfirmFrame);
	return FMath::Max(0, FMath::Max(1, RollbackFrameCount) - MaxRollbackFrame) + 1;
}

void URecallMultiSimSubsystem::Rollback_Step(int32 Step)
{
	if (ensure(StepThread.IsValid()))
	{
		StepThread->Rollback_Step(Step);
	}
}

void URecallMultiSimSubsystem::RenderSimulation(float DeltaTime, bool bResetRender /*= false*/)
{
	FScopeLock SimulationLock(&SimulationGuard);

	checkf(HasSimulationStarted(), TEXT("Can not render the simulation until it has started."));

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (!bDebugRenderSimulation)
	{
		return;
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	if (!bRenderSimulation)
	{
		return;
	}

	bRenderProcessingPhase = true;

	const TArray<const UWorld*> WorldsToRender = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	if (!WorldsToRender.IsEmpty())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallMultiSimSubsystem::Render"));
		QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_Render);

		for (const UWorld* World : WorldsToRender)
		{
			if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
			{
				if (SimulationSystem->ShouldRender())
				{
					SimulationSystem->SetSimulationRenderDeltaFrame(DeltaFrames);
					SimulationSystem->SetSimulationRenderSpeedScale(SpeedScale);
					SimulationSystem->SetResetRender(bResetRender);
					SimulationSystem->Render(DeltaTime);
					SimulationSystem->SetResetRender(false);
				}
			}
		}
	}

	bRenderProcessingPhase = false;
}

void URecallMultiSimSubsystem::PauseSimulation()
{
	WaitForStepThread();
	
	bPause = true;
}

void URecallMultiSimSubsystem::ResumeSimulation()
{
	WaitForStepThread();
	
	bPause = false;
}

bool URecallMultiSimSubsystem::TryPushLocalInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input)
{
	if (ensure(!PlayerId.IsEmpty()) == false)
	{
		return false;
	}

	if (IRecallInputQueueInterface* InputSystem = UWorld::GetSubsystem<URecallInputQueueSubsystem>(GetWorld()))
	{
		if (InputSystem->HasFrameInput(Frame, PlayerId) == false)
		{
			InputSystem->PushInput(Frame, PlayerId, Input);

			return true;
		}
	}

	return false;
}

void URecallMultiSimSubsystem::PushInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input)
{
	if (ensure(!PlayerId.IsEmpty()) == false)
	{
		return;
	}

	if (IRecallInputQueueInterface* InputSystem = UWorld::GetSubsystem<URecallInputQueueSubsystem>(GetWorld()))
	{
		InputSystem->PushInput(Frame, PlayerId, Input);
	}
}

void URecallMultiSimSubsystem::RequestAddPlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams)
{
	if (ensure(!PlayerId.IsEmpty()) == false)
	{
		return;
	}

	WaitForStepThread();

	const TArray<const UWorld*> Worlds = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	if (WorldIndex >= 0 && WorldIndex < Worlds.Num())
	{
		const UWorld* World = Worlds[WorldIndex];
		if (IRecallPlayerQueueInterface* PlayerQueueSystem = UWorld::GetSubsystem<URecallPlayerQueueSubsystem>(World))
		{
			PlayerQueueSystem->RequestAddPlayer(Frame, PlayerId, SpawnParams);
		}
	}
}

void URecallMultiSimSubsystem::RequestRemovePlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId)
{
	if (ensure(!PlayerId.IsEmpty()) == false)
	{
		return;
	}

	WaitForStepThread();

	const TArray<const UWorld*> Worlds = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	if (WorldIndex >= 0 && WorldIndex < Worlds.Num())
	{
		const UWorld* World = Worlds[WorldIndex];
		if (IRecallPlayerQueueInterface* PlayerQueueSystem = UWorld::GetSubsystem<URecallPlayerQueueSubsystem>(World))
		{
			PlayerQueueSystem->RequestRemovePlayer(Frame, PlayerId);
		}
	}
}

void URecallMultiSimSubsystem::RegisterObserver(UClass* Class, UObject* ObjectPointer)
{
	for (const UWorld* World : Recall::MultiWorld::Utils::GetMultiWorlds(this))
	{
		if (IRecallObserverSubjectInterface* ObserverSubjectSystem = UWorld::GetSubsystem<URecallObserverSubjectSubsystem>(World))
		{
			ObserverSubjectSystem->RegisterObserver(Class, ObjectPointer);
		}
	}
}

void URecallMultiSimSubsystem::UnregisterObserver(UObject* SourceObject)
{
	for (const UWorld* World : Recall::MultiWorld::Utils::GetMultiWorlds(this))
	{
		if (IRecallObserverSubjectInterface* ObserverSubjectSystem = UWorld::GetSubsystem<URecallObserverSubjectSubsystem>(World))
		{
			ObserverSubjectSystem->UnregisterObserver(SourceObject);
		}
	}
}

void URecallMultiSimSubsystem::SetFramesPerSecond(int32 FramesPerSecond)
{
	checkf(!HasSimulationStarted(), TEXT("Do not edit FPS after the game sim has started"));

	for (const UWorld* World : Recall::MultiWorld::Utils::GetMultiWorlds(this))
	{
		if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(World))
		{
			SimulationSystem->SetFramesPerSeconds(FramesPerSecond);
		}
	}
}

void URecallMultiSimSubsystem::WaitForStepThread() const
{
	if (StepThread.IsValid())
	{
		StepThread->WaitForStep();
	}
}

void URecallMultiSimSubsystem::OnLoadSnapshot(uint32 Frame, double TimeSeconds, bool bIsRollback)
{
	ElapsedFrame = Frame;
	ElapsedTime = TimeSeconds;

	if (!bIsRollback)
	{
		OnLoadSnapshotEvent.Broadcast(Frame);
		
		if (bRenderSimulation)
		{
			RenderSimulation(0.0f, true);
		}
	}
}
