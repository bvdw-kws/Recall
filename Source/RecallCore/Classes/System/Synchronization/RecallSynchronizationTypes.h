// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Input/RecallInputQueueTypes.h"

#include "RecallSynchronizationTypes.generated.h"

RECALLCORE_API extern bool IsRollback(const UObject* WorldContextObject);
RECALLCORE_API extern bool IsNetPause(const UObject* WorldContextObject);
RECALLCORE_API extern uint32 GetConfirmFrame(const UObject* WorldContextObject);
RECALLCORE_API extern int32 GetRollbackFrameCount(const UObject* WorldContextObject);

USTRUCT()
struct RECALLCORE_API FRecallSynchronizationWorldComparator
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallPlayerInput> PlayerInputs;
	
public:
	FString ToString() const;

public:
	bool operator==(const FRecallSynchronizationWorldComparator& Other) const;
	bool operator!=(const FRecallSynchronizationWorldComparator& Other) const { return !(*this == Other); }

};

USTRUCT()
struct RECALLCORE_API FRecallSynchronizationFrameComparator
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 Frame = 0;

	UPROPERTY(VisibleAnywhere, Transient)
	TArray<FRecallSynchronizationWorldComparator> WorldComparators;

public:
	FString ToString() const;

public:
	bool operator==(const FRecallSynchronizationFrameComparator& Other) const;
	bool operator!=(const FRecallSynchronizationFrameComparator& Other) const { return !(*this == Other); }

};

USTRUCT()
struct RECALLCORE_API FRecallSynchronizationSimulationComparator
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient)
	TArray<FRecallSynchronizationFrameComparator> FrameComparators;
	
public:
	bool IsEmpty() const { return FrameComparators.Num() == 0; }

	uint32 FindLastFrame() const;
	bool FindConfirmFrame(const FRecallSynchronizationSimulationComparator& Other, uint32 StartFrame, uint32& OutConfirmFrame) const;
	void GetFrameComparatorsFrom(uint32 Frame, TArray<FRecallSynchronizationFrameComparator>& OutComparators) const;

public:
	bool operator==(const FRecallSynchronizationSimulationComparator& Other) const { return FrameComparators == Other.FrameComparators; }
	bool operator!=(const FRecallSynchronizationSimulationComparator& Other) const { return !(*this == Other); }

};
