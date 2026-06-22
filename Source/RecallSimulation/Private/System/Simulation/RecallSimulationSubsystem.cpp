// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Simulation/RecallSimulationSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "MassProcessingPhaseManager.h"
#include "MassSimulationSubsystem.h"
#include "Settings/RecallSimulationSettings.h"
#include "Stats/Stats.h"
#include "Subsystems/SubsystemCollection.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Utility/Slowmo/RecallSlowmoUtils.h"

URecallSimulationSubsystem::URecallSimulationSubsystem()
	: Super()
{
}

void URecallSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UMassSimulationSubsystem>();

	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	SetFramesPerSeconds(SimulationSettings->FramesPerSeconds);

	UWorld* World = GetWorld();
	SimulationSystem = UWorld::GetSubsystem<UMassSimulationSubsystem>(World);
	
	if (World && World->IsGameWorld() && !World->IsPreviewWorld())
	{
		if (SimulationSystem.IsValid() && SimulationSystem->IsSimulationStarted())
		{
			SimulationSystem->GetMutablePhaseManager().DisableTickFunctions();
		}
		
		UMassSimulationSubsystem::GetOnSimulationStarted().AddUObject(this, &ThisClass::OnSimulationStarted);
	}
}

void URecallSimulationSubsystem::Deinitialize()
{
	Super::Deinitialize();

	SimulationSystem.Reset();

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && !World->IsPreviewWorld())
	{
		UMassSimulationSubsystem::GetOnSimulationStarted().RemoveAll(this);
	}
}

void URecallSimulationSubsystem::OnSimulationStarted(UWorld* World)
{
	if (SimulationSystem.IsValid() && SimulationSystem->IsSimulationStarted())
	{
		SimulationSystem->GetMutablePhaseManager().DisableTickFunctions();
	}
}

void URecallSimulationSubsystem::Start(const FRecallSimulationStartParams& Params)
{
	check(bStartedSimulation == false);

	bStartedSimulation = true;

	TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications = URecallSimulationReactSystemInterface::GetSimulationReactSystems(GetWorld());

	SimNotifications.StableSort([](const TScriptInterface<IRecallSimulationReactSystemInterface>& lhs, const TScriptInterface<IRecallSimulationReactSystemInterface>& rhs)
		{
			return lhs->GetStartOrderPriority() >= rhs->GetStartOrderPriority();
		}
	);

	for (const auto& SimNotif : SimNotifications)
	{
		SimNotif->Start(Params);
	}
}

void URecallSimulationSubsystem::Reset()
{
	SimulationTime = 0.0;
	SimulationFrame = 0;
	DilatedSimulationFrame = 0.0;

	const TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications = URecallSimulationReactSystemInterface::GetSimulationReactSystems(GetWorld());

	for (const auto& SimNotif : SimNotifications)
	{
		SimNotif->Reset();
	}

	bStartedSimulation = false;
}

void URecallSimulationSubsystem::OnTickStart()
{	
	FlushPersistentDebugLines(GetWorld());
}

void URecallSimulationSubsystem::StartFrame()
{
	check(bStartedSimulation);

	if (CheckTickSimulation() == false)
	{
		return;
	}

	const uint32 CurrentFrame = SimulationFrame;

	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Simulation_OnFrameStart);

		OnFrameStart.Broadcast(CurrentFrame);
	}
}

void URecallSimulationSubsystem::Step(EMassProcessingPhase Phase)
{
	check(bStartedSimulation);

	if (CheckTickSimulation() == false)
	{
		return;
	}

	const float FixedDeltaTime = GetFixedDeltaTime();

	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Simulation_StepProcessingPhase);

		FMassProcessingPhaseManager& PhaseManager = SimulationSystem->GetMutablePhaseManager();
		PhaseManager.Step(Phase, FixedDeltaTime);
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Simulation_OnProcessingPhase);

		OnProcessingPhase.Broadcast(GetWorld(), SimulationFrame, Phase);
	}
}

void URecallSimulationSubsystem::EndFrame()
{
	check(bStartedSimulation);

	if (CheckTickSimulation() == false)
	{
		return;
	}

	const uint32 CurrentFrame = SimulationFrame;
	const float FixedDeltaTime = GetFixedDeltaTime();

	SimulationTime += FixedDeltaTime;
	SimulationFrame++;
	DilatedSimulationFrame += Recall::Slowmo::Utils::GetTimeDilatation(this);

	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Simulation_OnFrameEnd);

		OnFrameEnd.Broadcast(CurrentFrame);
	}
}

void URecallSimulationSubsystem::Render(float DeltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Simulation_Render);

	if (CheckTickSimulation() == false)
	{
		return;
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Simulation_OnPreRender);

		OnPreRender.Broadcast();
	}

	FMassProcessingPhaseManager& PhaseManager = SimulationSystem->GetMutablePhaseManager();
	PhaseManager.Step(EMassProcessingPhase::Render, DeltaTime);
}

bool URecallSimulationSubsystem::CheckTickSimulation() const
{
	if (ensure(SimulationSystem.IsValid()) == false ||
		ensure(SimulationSystem->IsSimulationStarted()) == false)
	{
		return false;
	}
	
	return true;
}

void URecallSimulationSubsystem::OnLoadSnapshot(uint32 Frame, double TimeSeconds)
{
	SimulationFrame = Frame;
	DilatedSimulationFrame = static_cast<double>(Frame); // Note: Keep it synced (or just use it for representation?)
	SimulationTime = TimeSeconds;
}

void URecallSimulationSubsystem::SetShouldRender(bool bInShouldRender)
{ 
	if (bShouldRender == bInShouldRender)
	{
		return;
	}

	bShouldRender = bInShouldRender;

	OnChangeRenderState.Broadcast(bShouldRender);
}

void URecallSimulationSubsystem::SetFramesPerSeconds(int32 InFramesPerSeconds)
{
	checkf(!bStartedSimulation, TEXT("Do not edit FPS after the game sim has started"));

	FramesPerSeconds = InFramesPerSeconds; 
}

float URecallSimulationSubsystem::GetFixedDeltaTime() const
{
	return 1.0f / static_cast<float>(FramesPerSeconds);
}
