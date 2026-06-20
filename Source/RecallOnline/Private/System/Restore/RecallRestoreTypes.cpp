// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Restore/RecallRestoreTypes.h"

#include "MassEntityConfigAsset.h"
#include "RecallFrontendUtils.h"
#include "Serialization/MemoryWriter.h"
#include "System/Random/RecallRandomNumberInterface.h"
#include "Utility/MultiWorldUtils.h"

DEFINE_LOG_CATEGORY(LogRecallRestore);

bool FRecallRestorePlayerBunch::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << PlayerId;
	PlayerInput.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

bool FRecallRestoreInputBunch::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << StartFrame;
	Ar << FrameCount;

	uint8 PlayerCount = PlayerInputs.Num();
	Ar << PlayerCount;

	if (Ar.IsLoading())
	{
		PlayerInputs.SetNum(PlayerCount);
	}

	for (int32 PlayerIndex = 0; PlayerIndex < PlayerCount; PlayerIndex++)
	{
		PlayerInputs[PlayerIndex].NetSerialize(Ar, Map, bOutSuccess);
	}

	bOutSuccess = true;
	return true;
}

void FRecallRestoreWorldArray::GenerateFields(const UObject* WorldContextObject)
{
	checkf(!Items.Num(), TEXT("Only generate once"));

	const int32 WorldCount = MultiWorld::Utils::GetWorldCount(WorldContextObject);

	for (int32 WorldIndex = 0; WorldIndex < WorldCount; WorldIndex++)
	{
		FRecallWorldItemEntry& Item = Items.AddDefaulted_GetRef();

		if (const UWorld* World = MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex))
		{
			Item.Seed = Recall::Frontend::Utils::GetRefByWorld<IRecallRandomNumberInterface>(World).GetSeed();
		}
	}

	MarkArrayDirty();
}

/*
void FFRecallActorRepresentationDescInputItemEntry::PreReplicatedRemove(const FFRecallActorRepresentationDescInputArray& InArraySerializer)
{
}

void FFRecallActorRepresentationDescInputItemEntry::PostReplicatedAdd(const FFRecallActorRepresentationDescInputArray& InArraySerializer)
{
}

void FFRecallActorRepresentationDescInputItemEntry::PostReplicatedChange(const FFRecallActorRepresentationDescInputArray& InArraySerializer)
{
}

void FRecallRestorePlayerItemEntry::PreReplicatedRemove(const FRecallRestorePlayerArray& InArraySerializer)
{
}

void FRecallRestorePlayerItemEntry::PostReplicatedAdd(const FRecallRestorePlayerArray& InArraySerializer)
{
}

void FRecallRestorePlayerItemEntry::PostReplicatedChange(const FRecallRestorePlayerArray& InArraySerializer)
{
}
*/

FRecallPlayerQueueItem FRecallRestorePlayerItemEntry::GeneratePlayerQueueItem() const
{
	FRecallPlayerQueueItem Result;
	Result.EventType = EventType;
	Result.Frame = Frame;
	Result.PlayerId = PlayerId;
	Result.EntityConfig = Cast<UMassEntityConfigAsset>(SpawnParams.EntityConfigPath.TryLoad());

	return Result;
}

void FRecallRestorePlayerArray::PushAddPlayerItem(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams)
{
	FRecallRestorePlayerItemEntry& PlayerItem = Items.AddDefaulted_GetRef();
	PlayerItem.WorldIndex = WorldIndex;
	PlayerItem.EventType = ERecallPlayerQueueEvent::ADD;
	PlayerItem.Frame = Frame;
	PlayerItem.PlayerId = PlayerId;
	PlayerItem.SpawnParams = SpawnParams;

	MarkItemDirty(PlayerItem);
}

void FRecallRestorePlayerArray::PushRemovePlayerItem(int32 WorldIndex, uint32 Frame, const FString& PlayerId)
{
	FRecallRestorePlayerItemEntry& PlayerItem = Items.AddDefaulted_GetRef();
	PlayerItem.WorldIndex = WorldIndex;
	PlayerItem.EventType = ERecallPlayerQueueEvent::REMOVE;
	PlayerItem.Frame = Frame;
	PlayerItem.PlayerId = PlayerId;

	MarkItemDirty(PlayerItem);
}

void FRecallRestorePlayerArray::ResetPlayerItems()
{
	Items.Reset();
	MarkArrayDirty();
}

FRecallPlayerQueue FRecallRestorePlayerArray::GeneratePlayerQueue(int32 WorldIndex) const
{
	FRecallPlayerQueue PlayerQueue;

	for (const FRecallRestorePlayerItemEntry& Item : Items)
	{
		if (Item.WorldIndex == WorldIndex)
		{
			PlayerQueue.Queue.Add(Item.GeneratePlayerQueueItem());
		}
	}

	PlayerQueue.Queue.Sort([](const FRecallPlayerQueueItem& lhs, const FRecallPlayerQueueItem& rhs)
		{
			if (lhs.Frame == rhs.Frame)
			{
				return lhs.EventType < rhs.EventType;
			}
			else
			{
				return lhs.Frame < rhs.Frame;
			}
		});

	return PlayerQueue;
}

