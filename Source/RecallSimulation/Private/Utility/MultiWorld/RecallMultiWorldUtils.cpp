// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#ifdef WITH_MULTI_WORLD
#include "System/MultiWorldSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Utility/MultiWorldUtils.h"
#include "Player/MultiWorldLocalPlayer.h"
#endif // WITH_MULTI_WORLD

namespace Recall::MultiWorld::Utils
{

namespace
{
	ULocalPlayer* GetLocalPlayerFromContext(UObject* WorldContextObject, int32 LocalPlayerIndex)
	{
		if (WorldContextObject)
		{
			if (const UWorld* World = WorldContextObject->GetWorld())
			{
				if (const UGameInstance* GameInstance = World->GetGameInstance())
				{
					const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
					if (LocalPlayers.IsValidIndex(LocalPlayerIndex))
					{
						return LocalPlayers[LocalPlayerIndex];
					}
				}
			}
		}

		return nullptr;
	}

	const ULocalPlayer* GetLocalPlayerFromContext(const UObject* WorldContextObject, int32 LocalPlayerIndex)
	{
		return GetLocalPlayerFromContext(const_cast<UObject*>(WorldContextObject), LocalPlayerIndex);
	}
}

void InitializeMultiWorldDependency(FSubsystemCollectionBase& Collection)
{
#ifdef WITH_MULTI_WORLD
	Collection.InitializeDependency<UMultiWorldSubsystem>();
#endif // WITH_MULTI_WORLD
}

TWeakObjectPtr<UMultiWorldSubsystem> GetMultiWorldSystem(const UObject* WorldContextObject)
{
#ifdef WITH_MULTI_WORLD
	if (const UWorld* MainWorld = MultiWorld::Utils::GetMainWorld(WorldContextObject))
	{
		return UWorld::GetSubsystem<UMultiWorldSubsystem>(MainWorld);
	}
#endif // WITH_MULTI_WORLD
	return nullptr;
}

const UWorld* GetMainWorld(const UObject* WorldContextObject)
{
#ifdef WITH_MULTI_WORLD
	return MultiWorld::Utils::GetMainWorld(WorldContextObject);
#else // WITH_MULTI_WORLD
	return WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
#endif
}

bool IsMainWorld(const UObject* WorldContextObject)
{
#ifdef WITH_MULTI_WORLD
	return MultiWorld::Utils::IsMainWorld(WorldContextObject);
#else // WITH_MULTI_WORLD
	return true;
#endif
}

TArray<const UWorld*> GetMultiWorlds(const UObject* WorldContextObject)
{
	TArray<const UWorld*> Worlds;
#ifdef WITH_MULTI_WORLD
	if (const TWeakObjectPtr<UMultiWorldSubsystem> MultiWorldSystem = GetMultiWorldSystem(WorldContextObject))
	{
		if (MultiWorldSystem.IsValid())
		{
			for (UWorld* World : MultiWorldSystem->GetNestedWorlds())
			{
				Worlds.Add(World);
			}
		}
	}
#else // WITH_MULTI_WORLD
	if (WorldContextObject)
	{
		Worlds.Add(WorldContextObject->GetWorld());
	}
#endif
	return Worlds;
}

int32 GetWorldCount(const UObject* WorldContextObject)
{
#ifdef WITH_MULTI_WORLD
	return MultiWorld::Utils::GetWorldCount(WorldContextObject);
#else // WITH_MULTI_WORLD
	return WorldContextObject ? 1 : 0;
#endif
}

UWorld* GetWorldByIndex(const UObject* WorldContextObject, int32 WorldIndex)
{
#ifdef WITH_MULTI_WORLD
	return MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex);
#else // WITH_MULTI_WORLD
	if (WorldIndex == 0 && WorldContextObject)
	{
		return WorldContextObject->GetWorld();
	}
	return nullptr;
#endif
}

UWorld* GetMutableWorldByIndex(UObject* WorldContextObject, int32 WorldIndex)
{
#ifdef WITH_MULTI_WORLD
	return MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex);
#else // WITH_MULTI_WORLD
	if (WorldIndex == 0 && WorldContextObject)
	{
		return WorldContextObject->GetWorld();
	}
	return nullptr;
#endif
}

int32 GetWorldIndex(const UObject* WorldContextObject, const UWorld* World)
{
#ifdef WITH_MULTI_WORLD
	return MultiWorld::Utils::GetWorldIndex(WorldContextObject, World);
#else // WITH_MULTI_WORLD
	return World && WorldContextObject && World == WorldContextObject->GetWorld() ? 0 : INDEX_NONE;
#endif
}

int32 GetCurrentWorldIndex(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		return GetWorldIndex(WorldContextObject, WorldContextObject->GetWorld());
	}
	return INDEX_NONE;
}

FString GetCurrentWorldName(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		if (const UWorld* World = WorldContextObject->GetWorld())
		{
#ifdef WITH_MULTI_WORLD
			return MultiWorld::Utils::GetMapName(World);
#else // WITH_MULTI_WORLD
			return World->GetName();
#endif
		}
	}
	return FString();
}

void SubscribeOnAddNestedWorld(const UObject* WorldContextObject, const FRecallNestedWorldEvent& Callback)
{
#ifdef WITH_MULTI_WORLD
	if (UMultiWorldSubsystem* MultiWorldSystem = GetMultiWorldSystem(WorldContextObject).Get())
	{
		MultiWorldSystem->OnAddNestedWorld.Add(Callback);
	}
#endif // WITH_MULTI_WORLD
}

UWorld* GetLocalPlayerCurrentWorld(const APlayerController* PlayerController)
{
#ifdef WITH_MULTI_WORLD
	if (const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr)
	{
		if (const UMultiWorldLocalPlayer* MultiWorldLocalPlayer = Cast<UMultiWorldLocalPlayer>(LocalPlayer))
		{
			return MultiWorldLocalPlayer->GetCurrentWorld();
		}
	}
#endif // WITH_MULTI_WORLD

	return PlayerController ? PlayerController->GetWorld() : nullptr;
}

UWorld* GetLocalPlayerCurrentWorld(const UObject* WorldContextObject, int32 LocalPlayerIndex)
{
#ifdef WITH_MULTI_WORLD
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayerFromContext(WorldContextObject, LocalPlayerIndex))
	{
		if (const UMultiWorldLocalPlayer* MultiWorldLocalPlayer = Cast<UMultiWorldLocalPlayer>(LocalPlayer))
		{
			return MultiWorldLocalPlayer->GetCurrentWorld();
		}
	}
#endif // WITH_MULTI_WORLD

	return WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
}

bool SetLocalPlayerCurrentWorld(APlayerController* PlayerController, UWorld* World)
{
#ifdef WITH_MULTI_WORLD
	if (ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr)
	{
		if (UMultiWorldLocalPlayer* MultiWorldLocalPlayer = Cast<UMultiWorldLocalPlayer>(LocalPlayer))
		{
			MultiWorldLocalPlayer->SetCurrentWorld(World);
			return true;
		}
	}
	return false;
#else // WITH_MULTI_WORLD
	return IsValid(World);
#endif // WITH_MULTI_WORLD
}

bool SetLocalPlayerCurrentWorld(UObject* WorldContextObject, UWorld* World, int32 LocalPlayerIndex)
{
#ifdef WITH_MULTI_WORLD
	if (ULocalPlayer* LocalPlayer = GetLocalPlayerFromContext(WorldContextObject, LocalPlayerIndex))
	{
		if (UMultiWorldLocalPlayer* MultiWorldLocalPlayer = Cast<UMultiWorldLocalPlayer>(LocalPlayer))
		{
			MultiWorldLocalPlayer->SetCurrentWorld(World);
			return true;
		}
	}
	return false;
#else // WITH_MULTI_WORLD
	return IsValid(World);
#endif // WITH_MULTI_WORLD
}

} // namespace Recall::MultiWorld::Utils
