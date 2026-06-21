// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Input/RecallInputQueueSubsystem.h"

#include "RecallInputQueueSnapshot.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

void URecallInputQueueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Recall::MultiWorld::Utils::InitializeMultiWorldDependency(Collection);

	if (Recall::MultiWorld::Utils::IsMainWorld(this))
	{
		InputQueue = MakeShared<FRecallInputQueue>();
		PlayerControlLock = MakeShared<FRecallPlayerInputLock>();

		for (const UWorld* NestedWorld : Recall::MultiWorld::Utils::GetMultiWorlds(this))
		{
			RegisterNestedWorld(NestedWorld);
		}

		Recall::MultiWorld::Utils::SubscribeOnAddNestedWorld(
			this,
			FRecallNestedWorldEvent::CreateUObject(this, &ThisClass::OnAddNestedWorld));
	}
}

void URecallInputQueueSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallInputQueueSubsystem::OnAddNestedWorld(UWorld* World) const
{
	RegisterNestedWorld(World);
}

void URecallInputQueueSubsystem::RegisterNestedWorld(const UWorld* World) const
{
	if (URecallInputQueueSubsystem* InputSystem = UWorld::GetSubsystem<URecallInputQueueSubsystem>(World))
	{
		InputSystem->InputQueueCopy = InputQueue;
		InputSystem->PlayerControlLockCopy = PlayerControlLock;
	}
}

void URecallInputQueueSubsystem::Reset()
{
	if (InputQueue.IsValid())
	{
		(*InputQueue) = FRecallInputQueue();
	}

	if (PlayerControlLock.IsValid())
	{
		(*PlayerControlLock) = FRecallPlayerInputLock();
	}
}

void URecallInputQueueSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallInputQueueSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_InputQueue_Save);

	OutSnapshot.InitializeAs<FRecallInputQueueSnapshot>();

	FRecallInputQueueSnapshot& Snasphot = OutSnapshot.GetMutable<FRecallInputQueueSnapshot>();

	{
		FScopeLock Lock(&DataGuard);
	
		if (PlayerControlLock.IsValid())
		{
			Snasphot.PlayerInputLock = *PlayerControlLock.Get();
		}

		if (InputQueue.IsValid())
		{
			Snasphot.InputQueue = *InputQueue.Get();

			if (Context.IsSnapshot())
			{
				Snasphot.InputQueue.RemoveInputPastFrame(Context.Frame);
			}
		}
	}
}

void URecallInputQueueSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallInputQueueSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_InputQueue_Restore);

	if (const FRecallInputQueueSnapshot* InData = InSnapshot.GetPtr<FRecallInputQueueSnapshot>())
	{
		FScopeLock Lock(&DataGuard);
		
		if (PlayerControlLock.IsValid())
		{
			(*PlayerControlLock) = InData->PlayerInputLock;
		}

		if (Context.IsSnapshot() && InputQueue.IsValid())
		{
			(*InputQueue.Get()) = InData->InputQueue;
		}
	}
}

void URecallInputQueueSubsystem::SetInputQueue(const FRecallInputQueue& InInputQueue)
{ 
	checkf(InputQueue.IsValid() && IsInGameThread(),
		TEXT("%hs Can only be done in Main World"), __FUNCTION__);

	(*InputQueue) = InInputQueue;
}

const FRecallInputQueue& URecallInputQueueSubsystem::GetInputQueue() const
{
	return const_cast<URecallInputQueueSubsystem*>(this)->GetMutableInputQueue();
}

FRecallInputQueue& URecallInputQueueSubsystem::GetMutableInputQueue()
{
	checkf(InputQueueCopy.IsValid(),
		TEXT("%hs Input queue should be valid"), __FUNCTION__);
	
	return *InputQueueCopy.Pin();
}

void URecallInputQueueSubsystem::PushInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input)
{
	FRecallFrameInput FrameInput;
	FrameInput.Frame = Frame;
	FrameInput.Input = Input;

	PushFrameInput(PlayerId, FrameInput);
}

void URecallInputQueueSubsystem::PushFrameInput(const FString& PlayerId, const FRecallFrameInput& FrameInput)
{
	FScopeLock Lock(&DataGuard);
	
	FRecallInputQueuePlayerEntry& PlayerEntry = GetOrAddPlayerQueue(PlayerId);
	PlayerEntry.Queue.PushFrameInput(FrameInput);
}

bool URecallInputQueueSubsystem::HasFrameInput(uint32 Frame, const FString& PlayerId) const
{
	FRecallInput Input;
	return GetFrameInput(Frame, PlayerId, Input);
}

bool URecallInputQueueSubsystem::GetFrameInput(const FString& PlayerId, FRecallInput& OutInput, bool bAllowNone /*= true*/) const
{
	const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());
	return GetFrameInput(Frame, PlayerId, OutInput, bAllowNone);
}

TArray<FRecallPlayerInput> URecallInputQueueSubsystem::GetFramePlayerInputs(uint32 Frame, const TArray<FString>& PlayerIds, bool bAllowNone /*= true*/) const
{
	TArray<FRecallPlayerInput> PlayerInputs;

	for (const FString& PlayerId : PlayerIds)
	{
		FRecallInput Input;
		if (GetFrameInput(Frame, PlayerId, Input, bAllowNone))
		{
			FRecallPlayerInput PlayerInput;
			PlayerInput.PlayerId = PlayerId;
			PlayerInput.Input = Input;

			PlayerInputs.Add(PlayerInput);
		}
	}

	return PlayerInputs;
}

bool URecallInputQueueSubsystem::GetFrameInput(uint32 Frame, const FString& PlayerId, FRecallInput& OutInput, bool bAllowNone /*= true*/) const
{
	{
		FScopeLock Lock(&DataGuard);
	
		if (const FRecallInputQueuePlayerEntry* PlayerEntry = GetInputQueue().FindPlayerEntry(PlayerId))
		{
			const TArray<FRecallInputQueueItemEntry>& Items = PlayerEntry->Queue.GetItems();

			for (int32 ControlIndex = Items.Num() - 1; ControlIndex >= 0; ControlIndex--)
			{
				if (Items[ControlIndex].IsDuringFrame(Frame))
				{
					OutInput = Items[ControlIndex].Input;
					return true;
				}
				else if (Items[ControlIndex].IsBeforeFrame(Frame))
				{
					if (!bAllowNone)
					{
						OutInput = Items[ControlIndex].Input;
						return true;
					}
					break;
				}
			}
		}
	}

	if (!bAllowNone)
	{
		OutInput = FRecallInput();
		return true;
	}

	return false;
}

TArray<FRecallPlayerInput> URecallInputQueueSubsystem::GetFramePlayerInputs(uint32 Frame) const
{
	TArray<FRecallPlayerInput> PlayerInputs;

	{
		FScopeLock Lock(&DataGuard);
	
		for (const FRecallInputQueuePlayerEntry& PlayerEntry : GetInputQueue().PlayerEntries)
		{
			FRecallInput Input;
			if (GetFrameInput(Frame, PlayerEntry.PlayerId, Input))
			{
				FRecallPlayerInput PlayerInput;
				PlayerInput.PlayerId = PlayerEntry.PlayerId;
				PlayerInput.Input = Input;

				PlayerInputs.Add(PlayerInput);
			}
		}
	}

	return PlayerInputs;
}

FRecallInputQueuePlayerEntry& URecallInputQueueSubsystem::GetOrAddPlayerQueue(const FString& PlayerId)
{
	if (FRecallInputQueuePlayerEntry* PlayerEntry = GetMutableInputQueue().FindPlayerEntry(PlayerId))
	{
		return *PlayerEntry;
	}
	else
	{
		FRecallInputQueuePlayerEntry NewPlayerEntry;
		NewPlayerEntry.PlayerId = PlayerId;

		int32 ItemIndex = GetMutableInputQueue().PlayerEntries.Add(NewPlayerEntry);
		return GetMutableInputQueue().PlayerEntries[ItemIndex];
	}
}

void URecallInputQueueSubsystem::ClearInputQueuePastFrame(uint32 Frame)
{
	checkf(InputQueue.IsValid() && IsInGameThread(),
		TEXT("%hs Can only be done in Main World"), __FUNCTION__);
	
	FScopeLock Lock(&DataGuard);

	for (FRecallInputQueuePlayerEntry& PlayerEntry : GetMutableInputQueue().PlayerEntries)
	{
		PlayerEntry.Queue.RemoveAllAfterFrame(Frame);
	}
}

void URecallInputQueueSubsystem::SetLockControl(bool bLock)
{
	FScopeLock Lock(&DataGuard);
	check(PlayerControlLockCopy.IsValid());
	PlayerControlLockCopy.Pin()->bLocked = bLock;
}

bool URecallInputQueueSubsystem::IsLockControl() const
{
	FScopeLock Lock(&DataGuard);
	check(PlayerControlLockCopy.IsValid());
	return PlayerControlLockCopy.Pin()->bLocked;
}
