// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "RecallSlowMotionTypes.h"

#include "RecallSlowMotionSubsystem.generated.h"

UCLASS()
class RECALLSIMULATION_API URecallSlowMotionSubsystem :
	public UWorldSubsystem,
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	void AddSlowMotionEvent(float TimeDilatation, int32 Duration);
	void AddSlowMotionEvent(const TObjectPtr<class UCurveFloat>& TimeDilatationCurve, int32 Duration);
	void ClearSlowMotionEvents();

	float GetTimeDilatation() const;

public:
	// Only call this method once at the start of the frame
	void UpdateTimeDilatation();

protected:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End
	
private:
	TSharedPtr<struct FRecallSlowMotionData> MainSlowMoData; // Only owned by main field
	TWeakPtr<struct FRecallSlowMotionData> SlowMoCopyData;

	mutable FCriticalSection DataGuard;

	UFUNCTION()
	void OnAddNestedWorld(UWorld* World);

	void RegisterNestedWorld(const UWorld* World);

	TArray<const UWorld*> GetMultiWorlds() const;
};

template<>
struct TMassExternalSubsystemTraits<URecallSlowMotionSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
