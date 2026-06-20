// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallPlayerQueueTypes.generated.h"

RECALLCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallPlayer, Log, All);

UENUM()
enum class ERecallPlayerQueueEvent : uint8
{
	ADD,
	REMOVE
};

UENUM()
enum class ERecallPlayerSpawnLocation : uint8
{
	// Spawn the player at the default player start.
	Default,
	
	// Spawn the player at the specified player start.
	PlayerStart,

	// Set the spawn location to the provided transform.
	Transform,
};

USTRUCT()
struct RECALLCORE_API FRecallPlayerSpawnParameters
{
	GENERATED_BODY()

	/**
	 * The entity config to spawn for this player.
	 */
	UPROPERTY(VisibleAnywhere, meta=(AllowedClasses="/Script/MassSpawner.MassEntityConfigAsset"))
	FSoftObjectPath EntityConfigPath;

	/**
	 * The spawn location to use for this player.
	 */
	UPROPERTY(VisibleAnywhere)
	ERecallPlayerSpawnLocation SpawnLocation = ERecallPlayerSpawnLocation::Default;
	
	/**
	 * The spawn transform of the player.
	 * Only used if bUsePlayerStart is false.
	 */
	UPROPERTY(VisibleAnywhere, meta=(EditCondition="SpawnLocation == ERecallPlayerSpawnLocation::Transform"))
	FTransform Transform;

	/**
	 * Specify the player start to use when spawned from a specific player start.
	 */
	UPROPERTY(VisibleAnywhere, meta=(EditCondition="SpawnLocation == ERecallPlayerSpawnLocation::PlayerStart"))
	FName PlayerStart = NAME_None;

	/**
	 * Custom spawn parameters that can be set from the game mode.
	 */
	UPROPERTY(VisibleAnywhere)
	FInstancedStruct CustomParams;

	bool operator!=(const FRecallPlayerSpawnParameters& Other) const { return !(*this == Other); }
	bool operator==(const FRecallPlayerSpawnParameters& Other) const
	{
		return EntityConfigPath == Other.EntityConfigPath
			&& SpawnLocation == Other.SpawnLocation
			&& FTransform::AreTranslationsEqual(Transform, Other.Transform)
			&& FTransform::AreRotationsEqual(Transform, Other.Transform)
			&& CustomParams == Other.CustomParams;
	}
};

USTRUCT()
struct FRecallPlayerQueueItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	ERecallPlayerQueueEvent EventType { ERecallPlayerQueueEvent::ADD };

	UPROPERTY(VisibleAnywhere)
	uint32 Frame { 0 };
	
	UPROPERTY(VisibleAnywhere)
	FString PlayerId;
	
	UPROPERTY(VisibleAnywhere, meta=(EditCondition="EventType == ERecallPlayerQueueEvent::ADD", EditConditionHides))
	TObjectPtr<class UMassEntityConfigAsset> EntityConfig;

	UPROPERTY(VisibleAnywhere, meta=(EditCondition="EventType == ERecallPlayerQueueEvent::ADD", EditConditionHides))
	FRecallPlayerSpawnParameters SpawnParameters;
	
	bool IsAddEvent() const { return EventType == ERecallPlayerQueueEvent::ADD; }
	bool IsRemoveEvent() const { return EventType == ERecallPlayerQueueEvent::REMOVE; }

	bool operator!=(const FRecallPlayerQueueItem& Other) const { return !(*this == Other); }
	bool operator==(const FRecallPlayerQueueItem& Other) const
	{
		return EventType == Other.EventType
			&& Frame == Other.Frame
			&& PlayerId == Other.PlayerId
			&& EntityConfig == Other.EntityConfig
			&& SpawnParameters == Other.SpawnParameters;
	}
};

USTRUCT()
struct RECALLCORE_API FRecallPlayerQueue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallPlayerQueueItem> Queue;

	bool operator!=(const FRecallPlayerQueue& Other) const { return !(*this == Other); }
	bool operator==(const FRecallPlayerQueue& Other) const
	{
		if (Queue.Num() != Other.Queue.Num())
		{
			return false;
		}

		for (int32 ItemIndex = 0; ItemIndex < Queue.Num(); ItemIndex++)
		{
			if (Queue[ItemIndex] != Other.Queue[ItemIndex])
			{
				return false;
			}
		}

		return true;
	}

};
