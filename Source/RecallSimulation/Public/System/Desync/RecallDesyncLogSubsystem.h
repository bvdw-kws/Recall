// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "System/Desync/RecallDesyncLogInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "Engine/World.h"
#include "Containers/RingBuffer.h"

#include "RecallDesyncLogSubsystem.generated.h"

USTRUCT()
struct FRecallFrameDesyncLines
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	uint32 Frame = 0;

	UPROPERTY(Transient)
	TArray<FString> Lines;
};

/**
* Generate a desync log file that can then be compared with other files to check where the game ran out of sync.
* We can use RECALL_DESYNC_LOG to log variables that we want to compare and track the variables that ran out of sync.
*/
UCLASS()
class RECALLSIMULATION_API URecallDesyncLogSubsystem : 
	public UWorldSubsystem, 
	public IRecallSimulationReactSystemInterface,
	public IRecallDesyncLogInterface
{
	GENERATED_BODY()

public:
	void Log(const FString& Format, const FString& Function);

protected:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Start(const FRecallSimulationStartParams& Params) override final;
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

	// IRecallDesyncLogInterface implementation Begin
	virtual void DumpDesyncLog() override final;
	// IRecallDesyncLogInterface implementation End

private:
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	mutable FCriticalSection DataGuard;
	TRingBuffer<FRecallFrameDesyncLines, TFixedAllocator<2048>> FrameLines;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	void TrimLog(uint32 Frame);

	FString GetFileName() const;

	UFUNCTION()
	void OnFrameEnd(uint32 Frame);
};

template<>
struct TMassExternalSubsystemTraits<URecallDesyncLogSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
