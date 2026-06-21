// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Player/RecallPlayerUtils.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Interface/RecallPlayerStateInterface.h"
#include "Simulation/Player/Controller/RecallPlayerControllerTypes.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "System/Input/RecallInputQueueSubsystem.h"
#include "System/Player/RecallPlayerQueueSubsystem.h"
#include "System/World/RecallWorldTransitionSubsystem.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

#define RECALL_PLAYER_ID_START_STR TEXT("Player")

namespace Recall::Player::Utils
{
	
FString GetPlayerIdFromIndex(int32 PlayerIndex /**= 0*/)
{
	return FString::Printf(TEXT("%s%d"), RECALL_PLAYER_ID_START_STR, PlayerIndex + 1);
}

int32 GetPlayerIndexFromId(FString PlayerID)
{
	PlayerID.RemoveFromStart(RECALL_PLAYER_ID_START_STR);
	return FCString::Atoi(*PlayerID) - 1;
}

FString GetPlayerId(const APlayerState* PlayerState)
{
	if (const IRecallPlayerStateInterface* PlayerStateInterface = Cast<IRecallPlayerStateInterface>(PlayerState))
	{
		return PlayerStateInterface->GetSimPlayerId();
	}
	else
	{
		return FString();
	}
}

static TArray<FGameplayTag> GetPlayerControllerTagList()
{
	// Add new tags here
	return {
		Controller_Player1,
		Controller_Player2,
		Controller_Player3,
		Controller_Player4,
	};
}
	
int32 GetPlayerIndexByControllerTag(const FGameplayTag& ControllerTag)
{
	const TArray<FGameplayTag> ControllerTags = GetPlayerControllerTagList();
	const int32 PlayerIndex = ControllerTags.IndexOfByKey(ControllerTag);
	if (PlayerIndex == INDEX_NONE)
	{
		checkNoEntry();
		return INDEX_NONE;
	}
	return PlayerIndex;
}

FString GetPlayerIdByControllerTag(const FGameplayTag& ControllerTag)
{
	const int32 PlayerIndex = GetPlayerIndexByControllerTag(ControllerTag);
	return Player::Utils::GetPlayerIdFromIndex(PlayerIndex);
}		
	
FGameplayTag GetPlayerControllerTagByIndex(int32 Index)
{
	const TArray<FGameplayTag> ControllerTags = GetPlayerControllerTagList();
	if (ControllerTags.IsValidIndex(Index))
	{
		return ControllerTags[Index];
	}
	else
	{
		checkNoEntry();
		return FGameplayTag::EmptyTag;
	}
}

FString GetLocalPlayerId(const UObject* WorldContextObject, int32 PlayerIndex /*= 0*/)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	check(World != nullptr);

	const UGameInstance* GameInstance = World->GetGameInstance();
	check(GameInstance != nullptr);

	const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
	if (LocalPlayers.IsValidIndex(PlayerIndex))
	{
		if (const APlayerController* PC = LocalPlayers[PlayerIndex]->PlayerController)
		{
			if (const APlayerState* PlayerState = PC->GetPlayerState<APlayerState>())
			{
				return GetPlayerId(PlayerState);
			}
			else
			{
				// Player State is not ready yet
				return FString();
			}
		}
		else
		{
			// In online multiplayer, what happens if the remote player does not have a PC locally?
			// Is it something that can happen?
			return GetPlayerIdFromIndex(PlayerIndex);
		}
	}
	else
	{
		// No local player at index
		return FString();
	}

	// checkNoEntry();
	// return FString();
}

int32 GetNumLocalPlayer(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	check(World != nullptr);
	return UGameplayStatics::GetNumLocalPlayerControllers(World->GetGameInstance());
}

TArray<FString> GetLocalPlayerIds(const UObject* WorldContextObject)
{
	TArray<FString> PlayerIds;

	for (int32 LocalPlayerIndex = 0; LocalPlayerIndex < GetNumLocalPlayer(WorldContextObject); LocalPlayerIndex++)
	{
		PlayerIds.Add(GetLocalPlayerId(WorldContextObject, LocalPlayerIndex));
	}

	return PlayerIds;
}

bool IsLocalPlayer(const UObject* WorldContextObject, const FString& PlayerId)
{
	return GetLocalPlayerIds(WorldContextObject).Contains(PlayerId);
}

bool FindLocalPlayerIndex(const UObject* WorldContextObject, const FString& PlayerId, int32& OutLocalPlayerIndex)
{
	const int32 NumLocalPlayer = GetNumLocalPlayer(WorldContextObject);

	for (int32 LocalPlayerIndex = 0; LocalPlayerIndex < NumLocalPlayer; LocalPlayerIndex++)
	{
		if (PlayerId == GetLocalPlayerId(WorldContextObject, LocalPlayerIndex))
		{
			OutLocalPlayerIndex = LocalPlayerIndex;
			return true;
		}
	}

	return false;
}

TArray<FString> GetPlayersInWorld(const UWorld* World)
{
	if (const URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World))
	{
		return EntitySystem->GetControllerIDs();
	}

	return {};
}

bool FindPlayerEntityInWorld(const UWorld* World, const FString& PlayerId, FMassEntityHandle& OutEntity)
{
	if (const URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World))
	{
		return EntitySystem->FindControllerOwnedEntity(PlayerId, OutEntity);
	}

	return false;
}
	
int32 GetNumPlayer(const UWorld* World)
{
	if (const URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World))
	{
		return EntitySystem->GetControllerCount();
	}
	return 0;
}

TArray<FString> FindPlayersInWorldAtFrame(const UWorld* World, uint32 Frame)
{
	const uint32 CurrentFrame = Recall::Simulation::Utils::GetFrame(World);
	check(Frame <= CurrentFrame);

	TArray<FString> PlayerIds = GetPlayersInWorld(World);

	// We want to include players that will be added this frame.
	if (const URecallPlayerQueueSubsystem* PlayerQueueSystem = UWorld::GetSubsystem<URecallPlayerQueueSubsystem>(World))
	{
		PlayerQueueSystem->ApplyFrameCommitToPlayerArray(PlayerIds);
	}

	// Rollback to get the players at past frame.
	const uint32 FrameRange = CurrentFrame - Frame + 1;

	for (uint32 RollbackFrameIndex = 0; RollbackFrameIndex < FrameRange; RollbackFrameIndex++)
	{
		uint32 RollbackFrame = CurrentFrame - RollbackFrameIndex;

		// Only rollback transition for frame other than current frame.
		if (RollbackFrame < CurrentFrame)
		{
			if (const URecallWorldTransitionSubsystem* WorldTransitionSystem = UWorld::GetSubsystem<URecallWorldTransitionSubsystem>(World))
			{
				WorldTransitionSystem->RollbackFrameForPlayerArray(RollbackFrame, PlayerIds);
			}
		}

		// Do not rollback player events for target frame.
		if (RollbackFrame > Frame)
		{
			if (const URecallPlayerQueueSubsystem* PlayerQueueSystem = UWorld::GetSubsystem<URecallPlayerQueueSubsystem>(World))
			{
				PlayerQueueSystem->RollbackFrameForPlayerArray(RollbackFrame, PlayerIds);
			}
		}
	}

	return PlayerIds;
}

bool FindLocalPlayerEntityInWorld(const UWorld* World, int32 PlayerIndex, FMassEntityHandle& OutEntity)
{
	const FString PlayerId = GetLocalPlayerId(World, PlayerIndex);
	return FindPlayerEntityInWorld(World, PlayerId, OutEntity);
}

int32 FindLocalPlayerWorldIndex(const UObject* WorldContextObject, int32 PlayerIndex)
{
	const FString PlayerId = GetLocalPlayerId(WorldContextObject, PlayerIndex);
	return GetPlayerWorldIndex(WorldContextObject, PlayerId);
}

TArray<FString> GetLocalPlayersInWorld(const UWorld* World)
{
	TArray<FString> PlayerIds = GetLocalPlayerIds(World);

	if (const URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World))
	{
		for (int32 PlayerIndex = PlayerIds.Num() - 1; PlayerIndex >= 0; PlayerIndex--)
		{
			if (EntitySystem->HasControllerOwnedEntity(PlayerIds[PlayerIndex]) == false)
			{
				PlayerIds.RemoveAt(PlayerIndex);
			}
		}
	}

	return PlayerIds;
}

int32 GetPlayerWorldIndex(const UObject* WorldContextObject, const FString& PlayerId)
{
	const TArray<const UWorld*> Worlds = Recall::MultiWorld::Utils::GetMultiWorlds(WorldContextObject);

	for (int32 WorldIndex = 0; WorldIndex < Worlds.Num(); WorldIndex++)
	{
		if (const URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(Worlds[WorldIndex]))
		{
			if (EntitySystem->HasControllerOwnedEntity(PlayerId))
			{
				return WorldIndex;
			}
		}
	}

	return INDEX_NONE;
}

TArray<FString> FindLocalPlayerInputPhase(const UObject* WorldContextObject, uint32 Frame, bool bSkipInput /*= false*/)
{
	TArray<FString> PlayerIds;

	const TArray<const UWorld*> Worlds = Recall::MultiWorld::Utils::GetMultiWorlds(WorldContextObject);

	for (const UWorld* World : Worlds)
	{
		TArray<FString> FieldPlayerIds = GetLocalPlayersInWorld(World);

		// We want to already trigger input phase for players that will be added this frame.
		if (URecallPlayerQueueSubsystem* PlayerQueueSystem = UWorld::GetSubsystem<URecallPlayerQueueSubsystem>(World))
		{
			PlayerQueueSystem->ApplyFrameCommitToPlayerArray(FieldPlayerIds, true);
		}

		PlayerIds.Append(FieldPlayerIds);
	}

	if (bSkipInput)
	{
		// We only keep players that do not have an input yet.
		const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
		if (const IRecallInputQueueInterface* InputSystem = UWorld::GetSubsystem<URecallInputQueueSubsystem>(MainWorld))
		{
			for (int32 PlayerIndex = PlayerIds.Num() - 1; PlayerIndex >= 0; PlayerIndex--)
			{
				const FString& PlayerId = PlayerIds[PlayerIndex];

				if (InputSystem->HasFrameInput(Frame, PlayerId))
				{
					PlayerIds.RemoveAt(PlayerIndex);
				}
			}
		}
	}

	return PlayerIds;
}

bool HasAnyLocalPlayerInputPhase(const UObject* WorldContextObject, uint32 Frame)
{
	return FindLocalPlayerInputPhase(WorldContextObject, Frame, true).Num() > 0;
}

} // namespace Recall::Player::Utils
