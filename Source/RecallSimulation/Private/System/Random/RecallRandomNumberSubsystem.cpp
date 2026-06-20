// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Random/RecallRandomNumberSubsystem.h"

#include "Utility/Simulation/RecallSimulationUtils.h"

DEFINE_LOG_CATEGORY(LogRecallRandomNumber);

URecallRandomNumberSubsystem::URecallRandomNumberSubsystem()
	: Super()
{
}

void URecallRandomNumberSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SetSeed(FMath::Rand());
}

void URecallRandomNumberSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallRandomNumberSubsystem::Reset()
{
	RandomStream = FRandomStream(InitialSeed);
}

void URecallRandomNumberSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRandomNumberSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_RandomNumber_Save);

	OutSnapshot.InitializeAs<FRecallRandomNumberSnapshot>();

	FRecallRandomNumberSnapshot& Snapshot = OutSnapshot.GetMutable<FRecallRandomNumberSnapshot>();
	Snapshot.InitialSeed = InitialSeed;
	Snapshot.CurrentSeed = RandomStream.GetCurrentSeed();
}

void URecallRandomNumberSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRandomNumberSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_RandomNumber_Restore);

	if (const FRecallRandomNumberSnapshot* InData = InSnapshot.GetPtr<FRecallRandomNumberSnapshot>())
	{
		InitialSeed = InData->InitialSeed;
		RandomStream = FRandomStream(InData->CurrentSeed);
	}
}

void URecallRandomNumberSubsystem::SetSeed(int32 InSeed)
{
	InitialSeed = InSeed;
	RandomStream = FRandomStream(InitialSeed);
}

const FRandomStream& URecallRandomNumberSubsystem::GetRandomStream()
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this,
		FString::Printf(TEXT("%hs Random stream should stay predictable"), __FUNCTION__));

	return RandomStream;
}

static float GetTotalWeight(const TArray<float>& Weights)
{
	float TotalWeight = 0.0f;

	for (const float Weight : Weights)
	{
		checkf(Weight >= 0.0f, TEXT("Weight must be positive"));
		TotalWeight += Weight;
	}

	return TotalWeight;
}

int32 URecallRandomNumberSubsystem::RandWeigthedIndex(const TArray<float>& Weights)
{
	const float TotalWeight = GetTotalWeight(Weights);
	if (TotalWeight == 0.0f)
	{
		return INDEX_NONE;
	}

	const float RandomWeight = GetRandomStream().FRandRange(0.0f, TotalWeight);

	float WeightSum = 0.0f;

	for (int32 WeightIndex = 0; WeightIndex < Weights.Num(); WeightIndex++)
	{
		const float Weight = Weights[WeightIndex];
		if (Weight == 0.0f)
		{
			continue;
		}

		WeightSum += Weight;

		if (RandomWeight <= WeightSum)
		{
			return WeightIndex;
		}
	}

	checkNoEntry();
	return INDEX_NONE;
}

TArray<int32> URecallRandomNumberSubsystem::GenerateDerangement(int32 Size)
{
	if (Size < 2)
	{
		// Return empty array for invalid sizes
		return TArray<int32>();
	}

	// Initialize indices array [0, 1, 2, ..., Size-1]
	TArray<int32> Indices;
	Indices.Reserve(Size);
	for (int32 Index = 0; Index < Size; ++Index)
	{
		Indices.Add(Index);
	}

	// For 2 elements, simple swap guarantees derangement
	if (Size == 2)
	{
		Indices.Swap(0, 1);
		return Indices;
	}

	// For 3+ elements, attempt to create derangement using shuffling
	bool bValidDerangement = false;
	int32 MaxAttempts = 100; // Prevent infinite loops
	
	while (!bValidDerangement && MaxAttempts > 0)
	{
		// Standard Fisher-Yates shuffle
		for (int32 ShuffleIndex = Size - 1; ShuffleIndex > 0; --ShuffleIndex)
		{
			const int32 SwapIndex = GetRandomStream().RandRange(0, ShuffleIndex);
			Indices.Swap(ShuffleIndex, SwapIndex);
		}
		
		// Check if it's a valid derangement (no element at its original position)
		bValidDerangement = true;
		for (int32 CheckIndex = 0; CheckIndex < Size; ++CheckIndex)
		{
			if (Indices[CheckIndex] == CheckIndex)
			{
				bValidDerangement = false;
				break;
			}
		}
		
		MaxAttempts--;
	}
	
	// Fallback: if we couldn't find derangement, force rotation to guarantee no element stays in place
	if (!bValidDerangement)
	{
		// Reset to original order
		for (int32 Index = 0; Index < Size; ++Index)
		{
			Indices[Index] = Index;
		}
		
		// Rotate all indices by 1 to guarantee no element stays in original position
		const int32 FirstIndex = Indices[0];
		for (int32 RotateIndex = 0; RotateIndex < Size - 1; ++RotateIndex)
		{
			Indices[RotateIndex] = Indices[RotateIndex + 1];
		}
		Indices[Size - 1] = FirstIndex;
	}

	return Indices;
}
