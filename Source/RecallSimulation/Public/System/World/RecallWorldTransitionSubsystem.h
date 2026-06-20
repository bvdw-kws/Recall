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
#include "RecallWorldTransitionTypes.h"

#include "RecallWorldTransitionSubsystem.generated.h"

/*
* Track world transition events, but the transition itself has to be custom
*/
UCLASS()
class RECALLSIMULATION_API URecallWorldTransitionSubsystem :
	public UWorldSubsystem, 
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

	URecallWorldTransitionSubsystem();

public:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	void Start(const FRecallSimulationStartParams& Params) override final;
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

	void SendEvent(uint32 Frame, const FString& PlayerId, ERecallWorldTransitionEventType EventType);

	/* Modify PlayerArray by reversing field transition events during the frame. */
	void RollbackFrameForPlayerArray(uint32 Frame, TArray<FString>& PlayerArray) const;

protected:
	/* Our field save data, such as the player previous location or transition events. */
	UPROPERTY(VisibleAnywhere, Transient)
	FRecallWorldTransitionData FieldTransitionData;

};

template<>
struct TMassExternalSubsystemTraits<URecallWorldTransitionSubsystem> final
{
	enum
	{
		GameThreadOnly = true
	};
};
