// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "System/Random/RecallRandomNumberInterface.h"
#include "Mass/ExternalSubsystemTraits.h"

#include "RecallRandomNumberSubsystem.generated.h"

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallRandomNumber, Log, All);

USTRUCT()
struct FRecallRandomNumberSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int32 InitialSeed = 0;

	UPROPERTY(VisibleAnywhere)
	int32 CurrentSeed = 0;
};

/**
* RandomStream for internal use in our simulation.
*/
UCLASS()
class RECALLSIMULATION_API URecallRandomNumberSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface, public IRecallRandomNumberInterface
{
	GENERATED_BODY()

	URecallRandomNumberSubsystem();

	// UWorldSubsystem implementation Begin
public:
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

	// IRecallRandomNumberInterface implementation Begin
	void SetSeed(int32 InSeed) override final;
	int32 GetSeed() const override final { return InitialSeed; }
	// IRecallRandomNumberInterface implementation End

	const FRandomStream& GetRandomStream();
	int32 GetCurrentSeed() const { return RandomStream.GetCurrentSeed(); }

	int32 RandWeigthedIndex(const TArray<float>& Weights);

	/**
	 * Generates a derangement of indices (0 to Size-1) where no element is in its original position.
	 * For arrays of size 2, performs a simple swap. For size 3+, attempts to find a valid derangement
	 * using shuffling, with a fallback rotation strategy to guarantee no element stays in place.
	 * @param Size The number of elements to derange
	 * @return Array of deranged indices, guaranteed to have no element at its original position
	 */
	TArray<int32> GenerateDerangement(int32 Size);

	/**
	 * Generates a derangement of the provided array where no element is in its original position.
	 * For arrays of size 2, performs a simple swap. For size 3+, attempts to find a valid derangement
	 * using shuffling, with a fallback rotation strategy to guarantee no element stays in place.
	 * @param InArray The array to derange (will be modified in place)
	 * @return Reference to the modified array for chaining
	 */
	template<typename T>
	TArray<T>& GenerateDerangement(TArray<T>& InArray);

protected:
	UPROPERTY(VisibleAnywhere, Transient)
	int32 InitialSeed{ 0 };

private:
	FRandomStream RandomStream;

};

template<>
struct TMassExternalSubsystemTraits<URecallRandomNumberSubsystem> final
{
	enum
	{
		GameThreadOnly = true
	};
};

template<typename T>
TArray<T>& URecallRandomNumberSubsystem::GenerateDerangement(TArray<T>& InArray)
{
	const int32 Size = InArray.Num();
	if (Size < 2)
	{
		// Return unchanged array for invalid sizes
		return InArray;
	}

	// Get deranged indices using the existing method
	const TArray<int32> DerangedIndices = GenerateDerangement(Size);
	
	// Store original array to apply derangement
	TArray<T> OriginalArray = InArray;
	
	// Apply the deranged indices to rearrange elements
	for (int32 Index = 0; Index < DerangedIndices.Num(); ++Index)
	{
		InArray[Index] = OriginalArray[DerangedIndices[Index]];
	}

	return InArray;
}
