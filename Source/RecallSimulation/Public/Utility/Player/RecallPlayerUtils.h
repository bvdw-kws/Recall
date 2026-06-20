// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FGameplayTag;
class APlayerState;
struct FMassEntityHandle;

namespace Recall::Player::Utils
{

RECALLSIMULATION_API extern FString GetPlayerIdFromIndex(int32 PlayerIndex = 0);
RECALLSIMULATION_API extern int32 GetPlayerIndexFromId(FString PlayerID);
RECALLSIMULATION_API extern FString GetPlayerId(const APlayerState* PlayerState);
	
// World
RECALLSIMULATION_API extern TArray<FString> GetPlayersInWorld(const UWorld* World);
RECALLSIMULATION_API extern TArray<FString> FindPlayersInWorldAtFrame(const UWorld* World, uint32 Frame);
RECALLSIMULATION_API extern int32 GetPlayerWorldIndex(const UObject* WorldContextObject, const FString& PlayerId);
RECALLSIMULATION_API extern bool FindPlayerEntityInWorld(const UWorld* World, const FString& PlayerId, FMassEntityHandle& OutEntity);
RECALLSIMULATION_API extern int32 GetNumPlayer(const UWorld* World);

// Local player methods have to be used with caution since their result won't be reliable for remote clients
RECALLSIMULATION_API extern FString GetLocalPlayerId(const UObject* WorldContextObject, int32 PlayerIndex = 0);
RECALLSIMULATION_API extern int32 GetNumLocalPlayer(const UObject* WorldContextObject);
RECALLSIMULATION_API extern TArray<FString> GetLocalPlayerIds(const UObject* WorldContextObject);
RECALLSIMULATION_API extern TArray<FString> GetLocalPlayersInWorld(const UWorld* World);
RECALLSIMULATION_API extern int32 FindLocalPlayerWorldIndex(const UObject* WorldContextObject, int32 PlayerIndex);
RECALLSIMULATION_API extern bool FindLocalPlayerEntityInWorld(const UWorld* World, int32 PlayerIndex, FMassEntityHandle& OutEntity);
RECALLSIMULATION_API extern bool IsLocalPlayer(const UObject* WorldContextObject, const FString& PlayerId);
RECALLSIMULATION_API extern bool FindLocalPlayerIndex(const UObject* WorldContextObject, const FString& PlayerId, int32& OutLocalPlayerIndex);

// Input Phase
RECALLSIMULATION_API extern TArray<FString> FindLocalPlayerInputPhase(const UObject* WorldContextObject, uint32 Frame, bool bSkipInput = false);
RECALLSIMULATION_API extern bool HasAnyLocalPlayerInputPhase(const UObject* WorldContextObject, uint32 Frame);

// Player tags
RECALLSIMULATION_API extern FGameplayTag GetPlayerControllerTagByIndex(int32 Index);
RECALLSIMULATION_API extern int32 GetPlayerIndexByControllerTag(const FGameplayTag& ControllerTag);
RECALLSIMULATION_API extern FString GetPlayerIdByControllerTag(const FGameplayTag& ControllerTag);
	
} // namespace Recall::Player::Utils
