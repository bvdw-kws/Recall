// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Player/RecallPlayerQueueSubsystem.h"

#include "Desync/RecallDesyncLog.h"
#include "Engine/World.h"
#include "MassEntityConfigAsset.h"
#include "Subsystems/SubsystemCollection.h"
#include "System/Player/RecallPlayerTypes.h"
#include "Utility/Player/RecallPlayerUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

void URecallPlayerQueueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URecallPlayerQueueSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallPlayerQueueSubsystem::RequestAddPlayer(uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams)
{
	FRecallPlayerQueueItem Item;
	Item.EventType = ERecallPlayerQueueEvent::ADD;
	Item.Frame = Frame;
	Item.PlayerId = PlayerId;
	Item.EntityConfig = Cast<UMassEntityConfigAsset>(SpawnParams.EntityConfigPath.TryLoad());
	Item.SpawnParameters = SpawnParams;

	PushPlayerRequest(Item);
}

void URecallPlayerQueueSubsystem::RequestRemovePlayer(uint32 Frame, const FString& PlayerId)
{
	FRecallPlayerQueueItem Item;
	Item.EventType = ERecallPlayerQueueEvent::REMOVE;
	Item.Frame = Frame;
	Item.PlayerId = PlayerId;

	PushPlayerRequest(Item);
}

void URecallPlayerQueueSubsystem::PushPlayerRequest(const FRecallPlayerQueueItem& Item)
{
	if (Item.PlayerId.IsEmpty())
	{
		UE_LOG(LogRecallPlayer, Error, TEXT("%hs Player Id is empty"), __FUNCTION__);
		return;
	}

	PlayerQueue.EventQueue.Queue.Add(Item);
	PlayerQueue.EventQueue.Queue.Sort([](const FRecallPlayerQueueItem& lhs, const FRecallPlayerQueueItem& rhs)
		{
			if (lhs.Frame == rhs.Frame)
			{
				return lhs.EventType < rhs.EventType;
			}
			else
			{
				return lhs.Frame < rhs.Frame;
			}
		}
	);
}

bool URecallPlayerQueueSubsystem::PopNextPlayerEvent(FRecallPlayerEvent& OutEvent)
{
	FScopeLock Lock(&DataGuard);
	if (PlayerQueue.EventQueue.Queue.IsValidIndex(PlayerQueue.CurrentItemIndex))
	{
		const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);
		const FRecallPlayerQueueItem& Item = PlayerQueue.EventQueue.Queue[PlayerQueue.CurrentItemIndex];

		check(Item.PlayerId.IsEmpty() == false);

		if (Frame == Item.Frame)
		{
			OutEvent.PlayerId = Item.PlayerId;
			OutEvent.bAddEvent = Item.IsAddEvent();
			OutEvent.EntityConfig = Item.EntityConfig;
			OutEvent.SpawnParameters = Item.SpawnParameters;

#if RECALL_DESYNC_LOG
			RECALL_DESYNC_LOG_STR(GetWorld(), PlayerQueue,
				FString::Printf(TEXT("Frame: %u, PlayerId: %s, AddEvent: %s"),
					Frame, *Item.PlayerId, Item.IsAddEvent() ? TEXT("true") : TEXT("false")));
#endif // RECALL_DESYNC_LOG

			PlayerQueue.CurrentItemIndex++;

			return true;
		}
	}

	return false;
}

void URecallPlayerQueueSubsystem::Reset()
{
	PlayerQueue = FRecallPlayerQueueState();
}

void URecallPlayerQueueSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPlayerQueueSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PlayerQueue_Save);

	OutSnapshot.InitializeAs<FRecallPlayerQueueState>(PlayerQueue);
}

void URecallPlayerQueueSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPlayerQueueSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PlayerQueue_Restore);

	if (const FRecallPlayerQueueState* InData = InSnapshot.GetPtr<FRecallPlayerQueueState>())
	{
		// During a rollback, we want to keep track of incoming events
		if (Context.IsRollback())
		{
			int32 NumPlayerQueue = PlayerQueue.EventQueue.Queue.Num();

			for (int32 ItemIndex = 0; ItemIndex < NumPlayerQueue; ItemIndex++)
			{
				const FRecallPlayerQueueItem& Item = PlayerQueue.EventQueue.Queue[ItemIndex];

				if (Item.Frame >= Context.Frame)
				{
					PlayerQueue.CurrentItemIndex = ItemIndex;
					break;
				}
			}
		}
		else if (Context.IsSnapshot())
		{
			PlayerQueue = *InData;
		}
	}
}

void URecallPlayerQueueSubsystem::ApplyFrameCommitToPlayerArray(TArray<FString>& PlayerIds, bool bLocalPlayer) const
{
	check(!Recall::Simulation::Utils::IsSimulationProcessingPhase(this));

	uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());

	int32 NumPlayerQueue = PlayerQueue.EventQueue.Queue.Num();

	for (int32 ItemIndex = NumPlayerQueue - 1; ItemIndex >= 0; ItemIndex--)
	{
		const FRecallPlayerQueueItem& Item = PlayerQueue.EventQueue.Queue[ItemIndex];

		if (Item.Frame < Frame)
		{
			break;
		}
		else if (Item.Frame == Frame)
		{
			if (bLocalPlayer && Recall::Player::Utils::IsLocalPlayer(this, Item.PlayerId) == false)
			{
				continue;
			}

			if (Item.IsAddEvent())
			{
				PlayerIds.Add(Item.PlayerId);
			}
			else if (Item.IsRemoveEvent())
			{
				PlayerIds.Remove(Item.PlayerId);
			}
		}
	}
}

void URecallPlayerQueueSubsystem::RollbackFrameForPlayerArray(uint32 Frame, TArray<FString>& PlayerIds) const
{
	check(!Recall::Simulation::Utils::IsSimulationProcessingPhase(this));

	int32 NumPlayerQueue = PlayerQueue.EventQueue.Queue.Num();

	for (int32 ItemIndex = NumPlayerQueue - 1; ItemIndex >= 0; ItemIndex--)
	{
		const FRecallPlayerQueueItem& Item = PlayerQueue.EventQueue.Queue[ItemIndex];

		if (Item.Frame < Frame)
		{
			break;
		}
		else if (Item.Frame == Frame)
		{
			if (Item.IsAddEvent())
			{
				PlayerIds.Remove(Item.PlayerId);
			}
			else if (Item.IsRemoveEvent())
			{
				PlayerIds.Add(Item.PlayerId);
			}
		}
	}
}

void URecallPlayerQueueSubsystem::ClearPlayerQueuePastFrame(uint32 Frame)
{
	const int32 NumPlayerQueue = PlayerQueue.EventQueue.Queue.Num();

	for (int32 ItemIndex = NumPlayerQueue - 1; ItemIndex >= 0; ItemIndex--)
	{
		const FRecallPlayerQueueItem& Item = PlayerQueue.EventQueue.Queue[ItemIndex];

		if (Item.Frame >= Frame)
		{
			PlayerQueue.CurrentItemIndex = ItemIndex;
			PlayerQueue.EventQueue.Queue.RemoveAt(ItemIndex);
		}
		else
		{
			break;
		}
	}
}

bool URecallPlayerQueueSubsystem::IsStartField(const FString& PlayerId) const
{
	FScopeLock Lock(&DataGuard);
	for (const FRecallPlayerQueueItem& Item : PlayerQueue.EventQueue.Queue)
	{
		if (Item.PlayerId == PlayerId && Item.IsAddEvent())
		{
			return true;
		}
	}

	return false;
}
