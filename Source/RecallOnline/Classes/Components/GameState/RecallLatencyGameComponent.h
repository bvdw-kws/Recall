// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallLatencyGameComponent.generated.h"

struct FRecallPlayerLatencyBunch;

struct FRecallNetworkLatencyEntry
{
	double LatencySeconds = 0.0;
	double FrameDrift = 0.0;
};

/**
 * Component to keep track of the network latency and frame drift.
 */
UCLASS()
class RECALLONLINE_API URecallLatencyGameComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	void ReceiveInputLatencyBunch(const FRecallPlayerLatencyBunch& LatencyBunch);

	/**
	 * Frame drift relative to the host.
	 */
	int32 GetFrameDrift() const;
	double GetDFrameDrift() const;
	
	/**
	 * Highest input latency from the most recent inputs.
	 */
	double GetInputNetworkLatencySpikeInSeconds() const;
	int32 GetInputNetworkLatencySpikeInFrames() const;
	
	/**
	 * Highest input frame drift, it can only be negative.
	 */
	double GetInputNetworkFrameDrift() const;

	uint32 GetNetPauseCounter() const { return NetPauseCounter; }	
	float EvaluateSimulationSpeedScale() const;

	/*
	 * How many frames to delay our input so it is synchronized on all the clients.
	 */
	uint32 EvaluateInputLatencyInFrames() const;
	float EvaluateInputSyncLatencyInSeconds() const;

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface

protected:
	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;
	
	/**
	 * Frame drift estimation.
	 * Only set for clients.
	 */
	UPROPERTY(Transient)
	double FrameDrift{ 0.0 };
	
	UPROPERTY(Transient)
	double SumFrameDrift{ 0.0 };
	UPROPERTY(Transient)
	double NumFrameDrift{ 0.0 };

	UPROPERTY(Transient)
	bool bNetPaused{ false };
	UPROPERTY(Transient)
	uint32 NetPauseCounter{ 0 };
	
	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> SimulationSpeedCurve;
		
	/**
	 * Buffer of the past network latency measurements.
	 */
	TCircularBuffer<FRecallNetworkLatencyEntry> NetworkLatencyEntries{ TCircularBuffer<FRecallNetworkLatencyEntry>(250) };

	/**
	 * Index of the next position of NetworkLatencyEntries to write to.
	 */
	int32 NetworkLatencyNextIndexToAddTo{ 0 };

	void OnSimTickStart(float DeltaTime);
	
	void PushNetworkLatency(double TimeSeconds, double Drift);
	void PushFrameDrift(double InFrameDrift);
};
