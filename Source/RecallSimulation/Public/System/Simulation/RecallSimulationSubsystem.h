// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RecallSimulationSubsystem.generated.h"

enum class EMassProcessingPhase : uint8;
struct FRecallSimulationStartParams;

/*
* Manage the simulation flow for this world.
*/
UCLASS()
class RECALLSIMULATION_API URecallSimulationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	URecallSimulationSubsystem();

	// UWorldSubsystem implementation Begin
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;

public:
	void Start(const FRecallSimulationStartParams& Params);
	void Reset();

	void OnTickStart();

	void StartFrame();
	void Step(EMassProcessingPhase Phase);
	void EndFrame();

	void Render(float DeltaTime);

	FORCEINLINE double GetSimulationTime() const { return SimulationTime.Load(); }
	FORCEINLINE uint32 GetSimulationFrame() const { return SimulationFrame.Load(); }
	FORCEINLINE double GetDilatedSimulationFrame() const { return DilatedSimulationFrame.Load(); }

	void SetSimulationRenderDeltaFrame(float DeltaFrame) { RenderSimulationDeltaFrame = DeltaFrame; }
	FORCEINLINE float GetSimulationRenderDeltaFrame() const { return RenderSimulationDeltaFrame; }

	void SetSimulationRenderSpeedScale(float SpeedScale) { RenderSimulationSpeedScale = SpeedScale; }
	FORCEINLINE float GetSimulationRenderSpeedScale() const { return RenderSimulationSpeedScale; }
	
	bool ShouldRender() const { return bShouldRender; }
	void SetShouldRender(bool bInShouldRender);

	void SetResetRender(bool bInIsResetRender) { bIsResetRender = bInIsResetRender; }
	FORCEINLINE bool IsResetRender() const { return bIsResetRender; }

	void SetFramesPerSeconds(int32 InFramesPerSeconds);
	FORCEINLINE int32 GetFramesPerSeconds() const { return FramesPerSeconds; }
	float GetFixedDeltaTime() const;

	void OnLoadSnapshot(uint32 Frame, double TimeSeconds);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFrameEvent, uint32 /*Frame*/);
	FOnFrameEvent OnFrameStart;
	FOnFrameEvent OnFrameEnd;

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnProcessingPhaseEvent, UWorld* /*World*/, uint32 /*Frame*/, EMassProcessingPhase /*Phase*/);
	FOnProcessingPhaseEvent OnProcessingPhase;

	DECLARE_MULTICAST_DELEGATE(FOnPreRenderEvent);
	FOnPreRenderEvent OnPreRender;
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeRenderStateEvent, bool /*bRender*/);
	FOnChangeRenderStateEvent OnChangeRenderState;

protected:
	UPROPERTY(VisibleAnywhere, Transient)
	int32 FramesPerSeconds = 60;

	UPROPERTY(VisibleAnywhere, Transient)
	bool bStartedSimulation = false;

	// Written from the multi-sim step thread and read from the game thread, so these need to be
	// atomic rather than plain UPROPERTY fields (UHT doesn't support TAtomic members, hence dropping
	// UPROPERTY here).
	TAtomic<double> SimulationTime{ 0.0 };

	TAtomic<uint32> SimulationFrame{ 0 };

	/**
	 * The frame of the simulation with time dilatation.
	 */
	TAtomic<double> DilatedSimulationFrame{ 0.0 };

	/**
	 * Keep track of how many frames have been stepped since the last render of the game simulation.
	 */
	UPROPERTY(VisibleAnywhere, Transient)
	float RenderSimulationDeltaFrame = 1.f;

	/**
	 * The speed at which the simulation is being updated.
	 */
	UPROPERTY(VisibleAnywhere, Transient)
	float RenderSimulationSpeedScale = 1.f;

	/**
	 * Whether the rendering of the simulation should be reset, such as after loading a snapshot.
	 */
	UPROPERTY(VisibleAnywhere, Transient)
	bool bIsResetRender = false;

	/**
	 * Toggle rendering the game simulation.
	 */	
	UPROPERTY(VisibleAnywhere, Transient)
	bool bShouldRender = true;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class UMassSimulationSubsystem> SimulationSystem;

	bool CheckTickSimulation() const;

	void OnSimulationStarted(UWorld* World);

};
