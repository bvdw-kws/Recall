// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Input/RecallInputQueueTypes.h"

#include "RecallInputNetSerializers.generated.h"

USTRUCT()
struct RECALLCORE_API FRecallPlayerInputMetadata
{
	GENERATED_BODY()

	/* Local frame to estimate our frame drift */
	UPROPERTY()
	uint32 LocalFrame{ 0 };

	/* Local confirm frame to update shared confirm frame */
	UPROPERTY()
	uint32 LocalConfirmFrame{ 0 };

	/* Local world times in seconds to estimate ping */
	UPROPERTY()
	float RealTimeSeconds{ 0.0f };

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

USTRUCT()
struct RECALLCORE_API FRecallPlayerInputBunch
{
	GENERATED_BODY()

	/* Game sim player ID of owner */
	UPROPERTY()
	FString PlayerId;

	/* Metadata of the input */
	UPROPERTY()
	FRecallPlayerInputMetadata Metadata;

	/* Inputs past the last confirm frames */
	UPROPERTY()
	TArray<FRecallFrameInput> FrameInputs;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FRecallPlayerInputBunch > : public TStructOpsTypeTraitsBase2< FRecallPlayerInputBunch >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

/**
 * Latency bunch is sent to other players, after received an input, to let them know how much time it took for
 * this input to travel back-and-forth between clients.
 */
USTRUCT()
struct RECALLCORE_API FRecallPlayerLatencyBunch
{
	GENERATED_BODY()

	/**
	 * Frame at which the original input was sent.
	 */
	UPROPERTY()
	uint32 InputSentFrame = 0 ;

	/**
	 * Time at which the original input was sent.
	 */
	UPROPERTY()
	float InputSentRealTimeSeconds = 0.0f;

	/**
	 * Confirmed frame of the player who received this input and sent this latency bunch.
	 */
	UPROPERTY()
	uint32 InputReceivedConfirmFrame = 0;
	
	/**
	 * Game simulation frame of the player who received this input and sent this latency bunch.
	 */
	UPROPERTY()
	uint32 InputReceivedFrame = 0;

	/**
	 * Whether the player who generated the latency bunch is the host.
	 */
	UPROPERTY()
	uint8 bInputReceivedHasAuthority = 0;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	FORCEINLINE bool DoesInputReceiverHasAuthority() const { return static_cast<bool>(bInputReceivedHasAuthority); }
};

template<>
struct TStructOpsTypeTraits< FRecallPlayerLatencyBunch > : public TStructOpsTypeTraitsBase2< FRecallPlayerLatencyBunch >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};
