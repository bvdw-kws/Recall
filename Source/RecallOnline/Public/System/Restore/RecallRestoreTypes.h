// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "System/Player/RecallPlayerQueueTypes.h"
#include "Online/RecallInputNetSerializers.h"

#include "RecallRestoreTypes.generated.h"

RECALLONLINE_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallRestore, Log, All);

USTRUCT()
struct RECALLONLINE_API FRecallRestorePlayerBunch
{
	GENERATED_BODY()

	UPROPERTY()
	FString PlayerId;
		
	UPROPERTY()
	FRecallPlayerInputBunch PlayerInput;

	bool operator==(const FString& OtherPlayerId) const
	{
		return PlayerId == OtherPlayerId;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FRecallRestorePlayerBunch > : public TStructOpsTypeTraitsBase2< FRecallRestorePlayerBunch >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

USTRUCT()
struct RECALLONLINE_API FRecallRestoreInputBunch
{
	GENERATED_BODY()
		
	UPROPERTY()
	uint32 StartFrame{ 0 };

	UPROPERTY()
	uint8 FrameCount{ 0 };
		
	UPROPERTY()
	TArray<FRecallRestorePlayerBunch> PlayerInputs;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FRecallRestoreInputBunch > : public TStructOpsTypeTraitsBase2< FRecallRestoreInputBunch >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

USTRUCT()
struct RECALLONLINE_API FRecallRestoreClientInfo
{
	GENERATED_BODY()

	/**
	 * Memory of the initial snapshot used to restore the state of the game simulation.
	 */
	UPROPERTY()
	TArray<uint8> SnapshotMemory;

	/**
	 * Game simulation frame at which the snapshot was taken.
	 */
	UPROPERTY()
	uint32 SnapshotFrame = 0;

	/**
	 * Next input bunch to send to the client.
	 * Only used while restoring the latest inputs.
	 */
	UPROPERTY()
	uint32 NextInputBunchStartFrame = 0;
	
	/**
	 * EasyDataTransfer handle for combined snapshot + input transfer (0 if not using combined transfer).
	 */
	UPROPERTY()
	int32 CombinedTransferHandle = 0;	

	/**
	 * Count of critical player events (see URecallPlayerSyncGateComponent) reflected by this snapshot,
	 * captured atomically alongside SnapshotMemory/SnapshotFrame. Used to seed the late-joining client's
	 * applied event count without racing events issued after the snapshot was taken.
	 */
	UPROPERTY()
	uint32 SnapshotEventCount = 0;

};

USTRUCT()
struct FRecallRestoreInfo
{
	GENERATED_BODY()

	UPROPERTY()
	bool bStarted = false;

	UPROPERTY()
	uint32 InputTargetFrame = 0;

	UPROPERTY()
	bool bSynced = false;

	/**
	 * Count of critical player events reflected by the snapshot this client is restoring from.
	 * Received from the server alongside the snapshot; see FRecallRestoreClientInfo::SnapshotEventCount.
	 */
	UPROPERTY()
	uint32 SnapshotEventCount = 0;
};

USTRUCT()
struct RECALLONLINE_API FRecallWorldItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Seed{ 0 };

	/** Optional functions you can implement for client side notification of changes to items */
	/*	
	void PreReplicatedRemove(const struct FRecallRestorePlayerArray& InArraySerializer);
	void PostReplicatedAdd(const struct FRecallRestorePlayerArray& InArraySerializer);
	void PostReplicatedChange(const struct FRecallRestorePlayerArray& InArraySerializer);
	*/
};

USTRUCT()
struct RECALLONLINE_API FRecallRestoreWorldArray : public FFastArraySerializer
{
	GENERATED_BODY()
		
	UPROPERTY()
	TArray<FRecallWorldItemEntry> Items;

	void GenerateFields(const UObject* WorldContextObject);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRecallWorldItemEntry, FRecallRestoreWorldArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits< FRecallRestoreWorldArray > : public TStructOpsTypeTraitsBase2< FRecallRestoreWorldArray >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};


USTRUCT()
struct RECALLONLINE_API FRecallRestorePlayerItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 WorldIndex{ INDEX_NONE };

	UPROPERTY()
	ERecallPlayerQueueEvent EventType{ ERecallPlayerQueueEvent::ADD };

	UPROPERTY()
	uint32 Frame{ 0 };
	
	UPROPERTY()
	FString PlayerId;

	UPROPERTY()
	FRecallPlayerSpawnParameters SpawnParams;

	FRecallPlayerQueueItem GeneratePlayerQueueItem() const;

	/** Optional functions you can implement for client side notification of changes to items */
	/*	
	void PreReplicatedRemove(const struct FRecallRestorePlayerArray& InArraySerializer);
	void PostReplicatedAdd(const struct FRecallRestorePlayerArray& InArraySerializer);
	void PostReplicatedChange(const struct FRecallRestorePlayerArray& InArraySerializer);
	*/
};

USTRUCT()
struct RECALLONLINE_API FRecallRestorePlayerArray: public FFastArraySerializer
{
	GENERATED_BODY()

public:
	void PushAddPlayerItem(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams);
	void PushRemovePlayerItem(int32 WorldIndex, uint32 Frame, const FString& PlayerId);
	void ResetPlayerItems();
	
	const TArray<FRecallRestorePlayerItemEntry>& GetItems() const { return Items; }

	FRecallPlayerQueue GeneratePlayerQueue(int32 WorldIndex) const;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRecallRestorePlayerItemEntry, FRecallRestorePlayerArray>(Items, DeltaParms, *this);
	}

private:	
	UPROPERTY()
	TArray<FRecallRestorePlayerItemEntry> Items;
};

template<>
struct TStructOpsTypeTraits< FRecallRestorePlayerArray > : public TStructOpsTypeTraitsBase2< FRecallRestorePlayerArray >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
