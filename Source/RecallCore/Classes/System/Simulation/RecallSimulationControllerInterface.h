// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallSimulationControllerInterface.generated.h"

struct FRecallInput;
struct FRecallPlayerSpawnParameters;
struct FRecallSimulationStartParams;

// TODO: Instead of ERecallWorldTransitionReason, use Options as FString
enum class ERecallWorldTransitionReason : uint8;

/**
* Interface to control the flow of the simulation
*/
UINTERFACE()
class RECALLCORE_API URecallSimulationControllerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallSimulationControllerInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void StartSimulation(const FRecallSimulationStartParams& Params, bool bResume = true) = 0;
	virtual void PauseSimulation() = 0;
	virtual void ResumeSimulation() = 0;
	virtual void ResetSimulation() = 0;
	virtual void RestartSimulation(const FRecallSimulationStartParams& Params, bool bResume = true) = 0;
	virtual bool IsSimulationPaused() const = 0;
	virtual bool HasSimulationStarted() const = 0;

	virtual double GetElapsedTime() const = 0;
	virtual uint32 GetElapsedFrame() const = 0;

	virtual void SetSpeedScale(float InSpeedScale) = 0;
	virtual void SetRenderSimulation(bool bInRenderSimulation) = 0;
	virtual void SetForceStepSim(uint32 Step = 1) = 0;

	virtual void PushInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input) = 0;
	virtual bool TryPushLocalInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input) = 0;

	virtual void RequestAddPlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams) = 0;
	virtual void RequestRemovePlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId) = 0;

	virtual void SetFramesPerSecond(int32 FramesPerSecond) = 0;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTickEvent, float /*DeltaTime*/);
	virtual FOnTickEvent& GetOnTickStartEvent() = 0;
	virtual FOnTickEvent& GetOnTickEndEvent() = 0;
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFrameEvent, uint32 /*Frame*/);
	virtual FOnFrameEvent& GetOnFrameStartEvent() = 0;
	virtual FOnFrameEvent& GetOnLoadSnapshotEvent() = 0;
	
	// DECLARE_MULTICAST_DELEGATE_FiveParams(FOnWorldTransitionEvent, const FString& /*PlayerId*/, UWorld* /*OldWorld*/, UWorld* /*NewWorld*/, ERecallWorldTransitionReason /*TransitionType*/, FName /*TransitionEffect*/);
	// virtual FOnWorldTransitionEvent& GetOnWorldTransitionEvent() = 0;
	
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInputPhaseEvent, uint32 /*Frame*/, const FString& /*PlayerId*/);
	virtual FOnInputPhaseEvent& GetOnInputPhaseEvent() = 0;

};
