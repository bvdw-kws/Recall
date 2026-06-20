// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallLatencyGameComponent.h"

#include "RecallFrontendUtils.h"
#include "RecallGameSimulationComponent.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallInputNetSerializers.h"
#include "Settings/RecallSimulationSettings.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "System/Synchronization/RecallSynchronizationInterface.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#include "Utility/Gameplay/RecallGameplayStatics.h"
#include "Utility/Synchronization/RecallSynchronizationUtils.h"

URecallLatencyGameComponent::URecallLatencyGameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{	
	for (uint32 Index = 0; Index < NetworkLatencyEntries.Capacity(); ++Index)
	{
		FRecallNetworkLatencyEntry& Entry = NetworkLatencyEntries[Index];
		Entry.LatencySeconds = 0.0;
		Entry.FrameDrift = 0.0;
	}
}

void URecallLatencyGameComponent::BeginPlay()
{
	Super::BeginPlay();
	
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	SimulationSpeedCurve = SimulationSettings->SimulationSpeedCurve.LoadSynchronous();
	
	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);
	
	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnTickStartEvent().AddUObject(this, &ThisClass::OnSimTickStart);
	}
}

void URecallLatencyGameComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnTickStartEvent().RemoveAll(this);
	}
	
	SimulationController = nullptr;
}

void URecallLatencyGameComponent::ReceiveInputLatencyBunch(const FRecallPlayerLatencyBunch& LatencyBunch)
{
	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float RealTimeSeconds = World->GetRealTimeSeconds();	
	
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	const uint32 Frame = GameSimulationComponent->GetLocalSimulationFrame();
	
	ensureAlwaysMsgf(Frame >= LatencyBunch.InputSentFrame,
		TEXT("%hs Is this client from the past?"), __FUNCTION__);
	
	const double EstimatedFrame = (static_cast<double>(LatencyBunch.InputSentFrame) + static_cast<double>(Frame)) * 0.5;
	const double NewFrameDrift = EstimatedFrame - static_cast<double>(LatencyBunch.InputReceivedFrame);
	
	// Network latency
	const double NetworkLatencySeconds = (RealTimeSeconds - static_cast<double>(LatencyBunch.InputSentRealTimeSeconds)) * 0.5;
	PushNetworkLatency(NetworkLatencySeconds, NewFrameDrift);

	// Only set the drift for the clients.
	if (!HasAuthority() && LatencyBunch.DoesInputReceiverHasAuthority())
	{
		PushFrameDrift(NewFrameDrift);
	}
}

void URecallLatencyGameComponent::OnSimTickStart(float DeltaTime)
{
	// Compensate Drift by adjusting simulation speed.
	{
		const float SimulationSpeedScale = EvaluateSimulationSpeedScale();

		if (IsValid(SimulationController.GetObject()))
		{
			SimulationController->SetSpeedScale(SimulationSpeedScale);
		}

		bool bShouldNetPause = IsNetPause(this);
		if (bShouldNetPause != bNetPaused)
		{
			bNetPaused = bShouldNetPause;
			if (bNetPaused)
			{
				NetPauseCounter++;
			}
		}
	}
}

void URecallLatencyGameComponent::PushNetworkLatency(double TimeSeconds, double Drift)
{	
	FRecallNetworkLatencyEntry& Entry = NetworkLatencyEntries[NetworkLatencyNextIndexToAddTo];
	Entry.LatencySeconds = TimeSeconds;
	Entry.FrameDrift = Drift;
	
	NetworkLatencyNextIndexToAddTo = NetworkLatencyEntries.GetNextIndex(NetworkLatencyNextIndexToAddTo);
}

void URecallLatencyGameComponent::PushFrameDrift(double InFrameDrift)
{
	// Accumulate the computed frame drift
	SumFrameDrift += InFrameDrift;
	NumFrameDrift += 1.0;

	// Reset the accumulated values to ensure that we remain representative of the current drift
	if (NumFrameDrift > 10)
	{
		SumFrameDrift /= NumFrameDrift;
		NumFrameDrift = 1.0;
	}

	double TargetFrameDrift = SumFrameDrift / NumFrameDrift;

	// Smoothly interpolate towards the new drift if we've already got one to avoid significant spikes
	if (FrameDrift == 0.0)
	{
		FrameDrift = TargetFrameDrift;
	}
	else
	{
		FrameDrift += (TargetFrameDrift - FrameDrift) * 0.5;
	}
}

int32 URecallLatencyGameComponent::GetFrameDrift() const
{
	const double DFrameDrift = GetDFrameDrift();
	return FMath::RoundToInt(DFrameDrift);
}

double URecallLatencyGameComponent::GetDFrameDrift() const
{
	if (HasAuthority())
	{
		// The host should only consider negative drift to catch up.
		return GetInputNetworkFrameDrift();
	}
	else
	{
		return FrameDrift;
	}
}

double URecallLatencyGameComponent::GetInputNetworkLatencySpikeInSeconds() const
{
	double NetworkLatency = 0.0;

	for (uint32 Index = 0; Index < NetworkLatencyEntries.Capacity(); ++Index)
	{
		NetworkLatency = FMath::Max(NetworkLatencyEntries[Index].LatencySeconds, NetworkLatency);
	}

	return NetworkLatency;
}

double URecallLatencyGameComponent::GetInputNetworkFrameDrift() const
{
	double NetworkFrameDrift = 0.0;

	for (uint32 Index = 0; Index < NetworkLatencyEntries.Capacity(); ++Index)
	{
		NetworkFrameDrift = FMath::Min(NetworkLatencyEntries[Index].FrameDrift, NetworkFrameDrift);
	}

	return NetworkFrameDrift;
}

int32 URecallLatencyGameComponent::GetInputNetworkLatencySpikeInFrames() const
{
	const double NetworkLatency = GetInputNetworkLatencySpikeInSeconds();
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const int32 FramesPerSecond = SimulationSettings->FramesPerSeconds;
	return FMath::CeilToInt(static_cast<double>(FramesPerSecond) * NetworkLatency);
}

float URecallLatencyGameComponent::EvaluateSimulationSpeedScale() const
{
	if (IsNetPause(this))
	{
		return 0.0f;
	}

	const float LocalFrameDrift = static_cast<float>(GetDFrameDrift());

	if (SimulationSpeedCurve)
	{
		return SimulationSpeedCurve->GetFloatValue(LocalFrameDrift);
	}
	else
	{
		if (LocalFrameDrift > 0.0f)
		{
			return 1.0f;
		}
		else
		{
			constexpr float SpeedCompensationRange = 0.1f;
			constexpr float DefaultMaxFrameDrift = 10.0f;
				
			const IRecallSynchronizationInterface& Sync = Recall::Synchronization::Utils::GetSynchronizationRef(this);
			const float RollbackFrameCount = static_cast<float>(Sync.GetRollbackFrameCount());
			const float MaxFrameDrift = RollbackFrameCount > 0.0f ? RollbackFrameCount : DefaultMaxFrameDrift;
			
			const float FrameDriftIntensity = FMath::Abs(LocalFrameDrift) / MaxFrameDrift;
			const float SpeedCompensationAlpha = FMath::Clamp(FrameDriftIntensity, 0.0f, 1.0f);
			const float SpeedCompensation = FMath::Lerp(0.0f, SpeedCompensationRange, SpeedCompensationAlpha);

			return 1.0f + SpeedCompensation;
		}
	}
}

uint32 URecallLatencyGameComponent::EvaluateInputLatencyInFrames() const
{
	// Send more frames of input while behind InputNetworkLatency
	const int32 NetworkLatency = GetInputNetworkLatencySpikeInFrames();
	const int32 NetworkFrameDrift = FMath::FloorToInt(GetInputNetworkFrameDrift());

	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const int32 InputNetworkLatencyBuffer = SimulationSettings->InputNetworkLatencyBuffer;
	const FInt32Interval& InputLatencyRange = SimulationSettings->InputLatencyRange;
	
	const int32 RollbackFrameCount = Recall::Synchronization::Utils::GetSynchronizationRef(this).GetRollbackFrameCount();

	// The input latency should be close to the network latency.
	// The frame drift is subtracted to compensate for the frame difference between clients.
	// The input network latency buffer is added for safety, to give us some room.
	// The rollback frame count is subtracted because the game simulation can be rollback if the input is received too late.
	const int32 InputLatency = NetworkLatency - NetworkFrameDrift + InputNetworkLatencyBuffer - RollbackFrameCount;
	
	const int32 ClampedInputLatency = FMath::Clamp(InputLatency, InputLatencyRange.Min, InputLatencyRange.Max);
	return static_cast<uint32>(ClampedInputLatency);
}

float URecallLatencyGameComponent::EvaluateInputSyncLatencyInSeconds() const
{
	const uint32 InputSyncLatency = EvaluateInputLatencyInFrames();
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const int32 FramesPerSecond = SimulationSettings->FramesPerSeconds;
	return static_cast<float>(InputSyncLatency) / static_cast<float>(FramesPerSecond);
}
