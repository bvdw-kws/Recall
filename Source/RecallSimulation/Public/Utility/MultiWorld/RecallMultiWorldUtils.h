// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

class UMultiWorldSubsystem;
class UWorld;
class UObject;
class FSubsystemCollectionBase;

DECLARE_DELEGATE_OneParam(FRecallNestedWorldEvent, UWorld*);

namespace Recall::MultiWorld::Utils
{

RECALLSIMULATION_API extern void InitializeMultiWorldDependency(FSubsystemCollectionBase& Collection);

RECALLSIMULATION_API extern TWeakObjectPtr<UMultiWorldSubsystem> GetMultiWorldSystem(const UObject* WorldContextObject);

RECALLSIMULATION_API extern const UWorld* GetMainWorld(const UObject* WorldContextObject);
RECALLSIMULATION_API extern bool IsMainWorld(const UObject* WorldContextObject);

RECALLSIMULATION_API extern TArray<const UWorld*> GetMultiWorlds(const UObject* WorldContextObject);
RECALLSIMULATION_API extern int32 GetWorldCount(const UObject* WorldContextObject);
RECALLSIMULATION_API extern const UWorld* GetWorldByIndex(const UObject* WorldContextObject, int32 WorldIndex);
RECALLSIMULATION_API extern UWorld* GetMutableWorldByIndex(UObject* WorldContextObject, int32 WorldIndex);
RECALLSIMULATION_API extern int32 GetWorldIndex(const UObject* WorldContextObject, const UWorld* World);
RECALLSIMULATION_API extern int32 GetCurrentWorldIndex(const UObject* WorldContextObject);

RECALLSIMULATION_API extern FString GetCurrentWorldName(const UObject* WorldContextObject);

RECALLSIMULATION_API extern void SubscribeOnAddNestedWorld(const UObject* WorldContextObject, const FRecallNestedWorldEvent& Callback);

} // namespace Recall::MultiWorld::Utils
