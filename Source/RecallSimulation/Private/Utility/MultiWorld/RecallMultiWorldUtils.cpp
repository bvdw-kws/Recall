// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

#include "Engine/World.h"

#ifdef WITH_MULTI_WORLD
#include "System/MultiWorldSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Utility/MultiWorldUtils.h"
#endif // WITH_MULTI_WORLD

namespace Recall::MultiWorld::Utils
{

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

const UWorld* GetWorldByIndex(const UObject* WorldContextObject, int32 WorldIndex)
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

} // namespace Recall::MultiWorld::Utils
