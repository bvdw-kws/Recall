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
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;

#if WITH_EDITOR
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
#endif // WITH_EDITOR
	// UWorldSubsystem implementation End

public:
	void Start(const FRecallSimulationStartParams& Params);
	void Reset();

	void OnTickStart();

	void StartFrame();
	void Step(EMassProcessingPhase Phase);
	void EndFrame();

	void Render(float DeltaTime);

	FORCEINLINE double GetSimulationTime() const { return SimulationTime; }
	FORCEINLINE uint32 GetSimulationFrame() const { return SimulationFrame; }
	FORCEINLINE double GetDilatedSimulationFrame() const { return DilatedSimulationFrame; }

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

	UPROPERTY(VisibleAnywhere, Transient)
	double SimulationTime = 0.0;

	UPROPERTY(VisibleAnywhere, Transient)
	uint32 SimulationFrame = 0;

	/**
	 * The frame of the simulation with time dilatation.
	 */
	UPROPERTY(VisibleAnywhere, Transient)
	double DilatedSimulationFrame = 0.0;

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
