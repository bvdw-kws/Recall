// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"

#include "RecallSimulationInsightTypes.generated.h"

RECALLCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallSimInsight, Log, All);

USTRUCT()
struct RECALLCORE_API FRecallSimulationFrameReport
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Transient)
	uint32 Frame{ 0 };

	UPROPERTY(VisibleAnywhere, Transient)
	int32 Seed{ 0 };

	UPROPERTY(VisibleAnywhere, Transient)
	uint32 StateTreeHash = 0;
	
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 MutableEntityCount = 0;
	
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 MutableEntityHash = 0;

	UPROPERTY(VisibleAnywhere, Transient)
	int32 SerialNumberGenerator { 0 };

	void LogDiff(const FRecallSimulationFrameReport& Other) const;

	bool operator!=(const FRecallSimulationFrameReport& Other) const { return !(*this == Other); }
	bool operator==(const FRecallSimulationFrameReport& Other) const
	{
		return Frame == Other.Frame
			&& Seed == Other.Seed
			&& StateTreeHash == Other.StateTreeHash
			&& MutableEntityCount == Other.MutableEntityCount
			&& MutableEntityHash == Other.MutableEntityHash
			&& SerialNumberGenerator == Other.SerialNumberGenerator;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	bool NetSerializeFromPrevious(const FRecallSimulationFrameReport& Previous, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

USTRUCT()
struct RECALLCORE_API FRecallSimulationFrameInsight
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 Frame{ 0 };
		
	UPROPERTY(VisibleAnywhere, Transient)
	TArray<FRecallSimulationFrameReport> Fields;

	void LogDiff(const FRecallSimulationFrameInsight& Other) const;

	bool operator!=(const FRecallSimulationFrameInsight& Other) const { return !(*this == Other); }
	bool operator==(const FRecallSimulationFrameInsight& Other) const
	{
		return Frame == Other.Frame
			&& Fields == Other.Fields;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	bool NetSerializeFromPrevious(const FRecallSimulationFrameInsight& Previous, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

USTRUCT()
struct RECALLCORE_API FRecallSimulationInsight
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere, Transient)
	TArray<FRecallSimulationFrameInsight> Frames;

	bool IsEmpty() const { return Frames.Num() == 0; }
	uint32 GetLastFrame() const;
	bool HasFrame(uint32 Frame) const;

	bool FindSyncedFrame(const FRecallSimulationInsight& Other, uint32 StartFrame, uint32& OutSyncedFrame) const;
	TArray<FRecallSimulationFrameInsight> GetFramesFrom(uint32 Frame) const;

	void LogDiff(const FRecallSimulationInsight& Other, uint32 StartFrame) const;

	bool operator!=(const FRecallSimulationInsight& Other) const { return !(*this == Other); }
	bool operator==(const FRecallSimulationInsight& Other) const
	{
		return Frames == Other.Frames;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

};

template<>
struct TStructOpsTypeTraits< FRecallSimulationInsight > : public TStructOpsTypeTraitsBase2< FRecallSimulationInsight >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};
