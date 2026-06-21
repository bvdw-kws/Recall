// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Input/RecallInputQueueInterface.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"

#include "RecallInputQueueSubsystem.generated.h"

USTRUCT()
struct FRecallPlayerInputLock
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	bool bLocked{ false };
};

UCLASS()
class RECALLSIMULATION_API URecallInputQueueSubsystem :
	public UWorldSubsystem, 
	public IRecallInputQueueInterface, 
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	bool GetFrameInput(const FString& PlayerId, FRecallInput& OutInput, bool bAllowNone = true) const;
	TArray<FRecallPlayerInput> GetFramePlayerInputs(uint32 Frame, const TArray<FString>& PlayerIds, bool bAllowNone = true) const;

	void SetLockControl(bool bLock);
	bool IsLockControl() const;

protected:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

	// IRecallInputQueueInterface implementation Begin
	virtual void SetInputQueue(const FRecallInputQueue& InInputQueue) override final;
	virtual const FRecallInputQueue& GetInputQueue() const override final;

	virtual bool GetFrameInput(uint32 Frame, const FString& PlayerId, FRecallInput& OutInput, bool bAllowNone = true) const override final;
	virtual TArray<FRecallPlayerInput> GetFramePlayerInputs(uint32 Frame) const override final;

	virtual void ClearInputQueuePastFrame(uint32 Frame) override final;

	virtual bool HasFrameInput(uint32 Frame, const FString& PlayerId) const override final;

	virtual void PushInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input) override final;
	virtual void PushFrameInput(const FString& PlayerId, const FRecallFrameInput& FrameInput) override final;
	// IRecallInputQueueInterface implementation End
	
	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

private:
	mutable FCriticalSection DataGuard;

	/* Input queue owned by the main field */
	TSharedPtr<FRecallInputQueue> InputQueue;

	/* Copy of the main input queue, stored in each field */
	TWeakPtr<FRecallInputQueue> InputQueueCopy;

	/* Lock owned by the main fields */
	TSharedPtr<FRecallPlayerInputLock> PlayerControlLock;

	/* Copy shared between fields */
	TWeakPtr<FRecallPlayerInputLock> PlayerControlLockCopy;

	FRecallInputQueue& GetMutableInputQueue();

	FRecallInputQueuePlayerEntry& GetOrAddPlayerQueue(const FString& PlayerId);

	UFUNCTION()
	void OnAddNestedWorld(UWorld* World) const;

	void RegisterNestedWorld(const UWorld* World) const;
};

template<>
struct TMassExternalSubsystemTraits<URecallInputQueueSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
