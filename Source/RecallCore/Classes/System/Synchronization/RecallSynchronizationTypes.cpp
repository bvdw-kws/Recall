// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Synchronization/RecallSynchronizationTypes.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RecallSynchronizationContainerSubsystem.h"

static const URecallSynchronizationContainerSubsystem* GetContainerSystem(const UObject* WorldContextObject)
{
	// Use Game Instance so we acces the main world
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : NULL;
	if (const URecallSynchronizationContainerSubsystem* ContainerSystem = UWorld::GetSubsystem<URecallSynchronizationContainerSubsystem>(World))
	{
		return ContainerSystem;
	}
	return NULL;
}

bool IsRollback(const UObject* WorldContextObject)
{
	if (const URecallSynchronizationContainerSubsystem* ContainerSystem = GetContainerSystem(WorldContextObject))
	{
		return ContainerSystem->IsRollback();
	}
	return false;
}

bool IsNetPause(const UObject* WorldContextObject)
{
	if (const URecallSynchronizationContainerSubsystem* ContainerSystem = GetContainerSystem(WorldContextObject))
	{
		return ContainerSystem->IsNetPause();
	}
	return false;
}

uint32 GetConfirmFrame(const UObject* WorldContextObject)
{
	if (const URecallSynchronizationContainerSubsystem* ContainerSystem = GetContainerSystem(WorldContextObject))
	{
		return ContainerSystem->GetConfirmFrame();
	}
	return 0;
}

int32 GetRollbackFrameCount(const UObject* WorldContextObject)
{
	if (const URecallSynchronizationContainerSubsystem* ContainerSystem = GetContainerSystem(WorldContextObject))
	{
		return ContainerSystem->GetRollbackFrameCount();
	}
	return 0;
}

FString FRecallSynchronizationWorldComparator::ToString() const
{
	FString Result;

	for (const FRecallPlayerInput& PlayerInput : PlayerInputs)
	{
		Result += FString::Printf(TEXT("	*%s\n"), *PlayerInput.ToString());
	}

	return Result;
}

bool FRecallSynchronizationWorldComparator::operator==(const FRecallSynchronizationWorldComparator& Other) const
{
	if (PlayerInputs.Num() != Other.PlayerInputs.Num())
	{
		return false;
	}

	for (int32 InputIndex = 0; InputIndex < PlayerInputs.Num(); InputIndex++)
	{
		if (PlayerInputs[InputIndex] != Other.PlayerInputs[InputIndex])
		{
			return false;
		}
	}

	return true;
}

FString FRecallSynchronizationFrameComparator::ToString() const
{
	FString Result = FString::Printf(TEXT("[%d] Frame comparator:\n"), Frame);

	for (int32 WorldIndex = 0; WorldIndex < WorldComparators.Num(); WorldIndex++)
	{
		Result += FString::Printf(TEXT("	[%d] Field:\n%s\n"), WorldIndex, *WorldComparators[WorldIndex].ToString());
	}

	return Result;
}

bool FRecallSynchronizationFrameComparator::operator==(const FRecallSynchronizationFrameComparator& Other) const
{
	return Frame == Other.Frame
		&& WorldComparators == Other.WorldComparators;
}

bool FRecallSynchronizationSimulationComparator::FindConfirmFrame(const FRecallSynchronizationSimulationComparator& Other, uint32 StartFrame, uint32& OutConfirmFrame) const
{
	TArray<FRecallSynchronizationFrameComparator> Comparators;
	GetFrameComparatorsFrom(StartFrame, Comparators);

	TArray<FRecallSynchronizationFrameComparator> OtherComparators;
	Other.GetFrameComparatorsFrom(StartFrame, OtherComparators);

	if (Comparators.Num() == 0 || OtherComparators.Num() == 0)
	{
		return false;
	}

	int32 FrameIndex = 0;

	if (Comparators[FrameIndex] != OtherComparators[FrameIndex])
	{
		return false;
	}
	
	const int32 FrameCount = FMath::Min(Comparators.Num(), OtherComparators.Num());

	do
	{
		OutConfirmFrame = Comparators[FrameIndex].Frame;
		FrameIndex++;
	} while (FrameIndex < FrameCount && Comparators[FrameIndex] == OtherComparators[FrameIndex]);

	return true;
}

uint32 FRecallSynchronizationSimulationComparator::FindLastFrame() const
{
	checkf(IsEmpty() == false, TEXT("We should not use an empty comparator."));
	return FrameComparators.Last().Frame;
}

void FRecallSynchronizationSimulationComparator::GetFrameComparatorsFrom(uint32 Frame, TArray<FRecallSynchronizationFrameComparator>& OutComparators) const
{
	OutComparators = FrameComparators.FilterByPredicate([Frame](const FRecallSynchronizationFrameComparator& FrameComparator)
		{
			return FrameComparator.Frame >= Frame;
		});
}
