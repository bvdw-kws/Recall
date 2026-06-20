// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Online/RecallVectorNetSerialization.h"
#include "NativeGameplayTags.h"

#include "RecallInputQueueTypes.generated.h"

RECALLCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallInput, Log, All);

RECALLCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_BlockControl);

/**
* Internal input commands, binding can be done through ARecallPlayerController::InputActionTable.
* Or it can be used directly by passing input commands to the internal simulation.
*/
UENUM(meta=(BitFlags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ERecallControllerInputCommand : uint16
{
	None	= 0,
	A		= 1 << 0		UMETA(Hidden),
	B		= 1 << 1		UMETA(Hidden),
	C		= 1 << 2		UMETA(Hidden),
	D		= 1 << 3		UMETA(Hidden),
	E		= 1 << 4		UMETA(Hidden),
	F		= 1 << 5		UMETA(Hidden),
	G		= 1 << 6		UMETA(Hidden),
	H		= 1 << 7		UMETA(Hidden),
	I		= 1 << 8		UMETA(Hidden),
	J		= 1 << 9		UMETA(Hidden),
	K		= 1 << 10		UMETA(Hidden),
	L		= 1 << 11		UMETA(Hidden),
	M		= 1 << 12		UMETA(Hidden),
	N		= 1 << 13		UMETA(Hidden),
	O		= 1 << 14		UMETA(Hidden),
	P		= 1 << 15		UMETA(Hidden),
};
ENUM_CLASS_FLAGS(ERecallControllerInputCommand)

constexpr bool EnumHasAnyFlags(uint16 Flags, ERecallControllerInputCommand Contains) { return (Flags & uint16(Contains)) != 0; }
inline uint16& operator|=(uint16& Lhs, ERecallControllerInputCommand Rhs) { return Lhs |= uint16(Rhs); }

#define MAX_RECALL_INPUT_COUNT 16

UENUM(BlueprintType)
enum class ERecallTransitionInputTrigger : uint8
{
	Press,
	Held,
	NotHeld,
};


USTRUCT()
struct RECALLCORE_API FRecallInput
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta=(Bitmask))
	uint16 InputCommand = 0;

	UPROPERTY(VisibleAnywhere)
	FVector2D_NetQuantizeDirection LeftStickAxis = FVector2D_NetQuantizeDirection::ZeroVector;

	UPROPERTY(VisibleAnywhere)
	FVector2D_NetQuantizeDirection RightStickAxis = FVector2D_NetQuantizeDirection::ZeroVector;
	
	UPROPERTY(VisibleAnywhere)
	TMap<uint16, FVector2D_NetQuantizeDirection> AxisInputMap;

	UPROPERTY(VisibleAnywhere)
	FVector2D_NetQuantize MousePosition = FVector2D_NetQuantizeDirection::ZeroVector;
	
	UPROPERTY(VisibleAnywhere)
	FString Options;

	bool operator==(const FRecallInput& Other) const;
	bool operator!=(const FRecallInput& Other) const { return !(*this == Other); }

	FString ToString() const;

	FORCEINLINE void ResetOptions() { Options = FString();}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/* Only NetSerialize what changed compare to the previous input */
	bool NetSerializeFromPrevious(const FRecallInput& PreviousInput, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FRecallInput > : public TStructOpsTypeTraitsBase2< FRecallInput >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

USTRUCT()
struct RECALLCORE_API FRecallFrameInput
{
	GENERATED_BODY()

	FRecallFrameInput() = default;
	FRecallFrameInput(uint32 InFrame, const FRecallInput& InInput);

	UPROPERTY(VisibleAnywhere)
	uint32 Frame { 0 };

	UPROPERTY(VisibleAnywhere)
	FRecallInput Input;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FRecallFrameInput > : public TStructOpsTypeTraitsBase2< FRecallFrameInput >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

/*
* Input stored using Run-length encoding algorithm
*/
USTRUCT()
struct RECALLCORE_API FRecallInputQueueItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	uint32 FrameStart { 0 };

	UPROPERTY(VisibleAnywhere)
	uint32 FrameCount { 1 };

	UPROPERTY(VisibleAnywhere)
	FRecallInput Input;

	uint32 LastFrame() const;

	bool IsNextFrame(uint32 Frame) const;
	bool IsDuringFrame(uint32 Frame) const;
	bool IsBeforeFrame(uint32 Frame) const;
	bool IsAfterFrame(uint32 Frame) const;

	bool operator!=(const FRecallInputQueueItemEntry& Other) const { return !(*this == Other); }
	bool operator==(const FRecallInputQueueItemEntry& Other) const
	{
		return FrameStart == Other.FrameStart
			&& FrameCount == Other.FrameCount
			&& Input == Other.Input;
	}
};

USTRUCT()
struct RECALLCORE_API FRecallInputQueueArray: public FFastArraySerializer
{
	GENERATED_BODY()
		
public:
	FORCEINLINE const TArray<FRecallInputQueueItemEntry>& GetItems() const { return Items; }

	bool PushFrameInput(const FRecallFrameInput& FrameInput);
	void RemoveAllAfterFrame(uint32 Frame);

	bool FindInput(uint32 Frame, FRecallInput& OutInput) const;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

	bool operator!=(const FRecallInputQueueArray& Other) const;
	bool operator==(const FRecallInputQueueArray& Other) const;

private:	
	UPROPERTY()
	TArray<FRecallInputQueueItemEntry> Items;
};

template<>
struct TStructOpsTypeTraits< FRecallInputQueueArray > : public TStructOpsTypeTraitsBase2< FRecallInputQueueArray >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

USTRUCT()
struct RECALLCORE_API FRecallInputQueuePlayerEntry
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FString PlayerId;

	UPROPERTY(VisibleAnywhere)
	FRecallInputQueueArray Queue;

	bool operator!=(const FRecallInputQueuePlayerEntry& Other) const { return !(*this == Other); }
	bool operator==(const FRecallInputQueuePlayerEntry& Other) const
	{
		return PlayerId == Other.PlayerId
			&& Queue == Other.Queue;
	}
};

USTRUCT()
struct RECALLCORE_API FRecallInputQueue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallInputQueuePlayerEntry> PlayerEntries;

	FRecallInputQueuePlayerEntry* FindPlayerEntry(const FString& PlayerId);
	const FRecallInputQueuePlayerEntry* FindPlayerEntry(const FString& PlayerId) const;

	void RemoveInputPastFrame(uint32 Frame);
	
	bool operator!=(const FRecallInputQueue& Other) const { return !(*this == Other); }
	bool operator==(const FRecallInputQueue& Other) const
	{
		if (PlayerEntries.Num() != Other.PlayerEntries.Num())
		{
			return false;
		}

		for (int32 EntryIndex = 0; EntryIndex < PlayerEntries.Num(); EntryIndex++)
		{
			if (PlayerEntries[EntryIndex] != Other.PlayerEntries[EntryIndex])
			{
				return false;
			}
		}

		return true;
	}

};

USTRUCT()
struct RECALLCORE_API FRecallPlayerInput
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FString PlayerId;

	UPROPERTY(VisibleAnywhere)
	FRecallInput Input;

	bool operator!=(const FRecallPlayerInput& Other) const { return !(*this == Other); }
	bool operator==(const FRecallPlayerInput& Other) const 
	{ 
		return PlayerId == Other.PlayerId 
			&& Input == Other.Input; 
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("[%s] %s"), *PlayerId, *Input.ToString());
	}

};
