// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "System/Simulation/Insight/RecallSimulationInsightInterface.h"
#include "Containers/RingBuffer.h"

#include "RecallSimulationInsightSubsystem.generated.h"

UCLASS()
class RECALLSIMULATION_API URecallSimulationInsightSubsystem : 
	public UWorldSubsystem,
	public IRecallSimulationReactSystemInterface,
	public IRecallSimulationInsightInterface
{
	GENERATED_BODY()

	URecallSimulationInsightSubsystem();

	// UWorldSubsystem implementation Begin
public:
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
public:
	void Start(const FRecallSimulationStartParams& Params) override final;
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

	// IRecallSimulationInsightInterface implementation Begin
public:
	void SetReportFromFrame(uint32 Frame) override final;
	FRecallSimulationInsight GenerateReportInRange(uint32 StartFrame, uint32 EndFrame) const override final;
	// IRecallSimulationInsightInterface implementation End

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallRandomNumberSubsystem> RandomNumberSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallStateTreeSubsystem> StateTreeSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallEntitySubsystem> EntitySystem;

	/* The oldest report frame. */
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 FromFrame{ 0 };

	mutable FCriticalSection DataGuard;
	TRingBuffer<FRecallSimulationFrameReport> LastFrames;

	void OnFrameStart(uint32 Frame);
	void OnFrameEnd(uint32 Frame);

	void SetReportFromFrame_Internal(uint32 Frame);

	bool FindFrameReport(uint32 Frame, FRecallSimulationFrameReport& OutReport) const;
	FRecallSimulationFrameReport GenerateFrameReport(uint32 Frame) const;
};
