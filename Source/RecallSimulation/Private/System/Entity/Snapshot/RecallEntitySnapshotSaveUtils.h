// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FMassArchetypeHandle;
struct FMassEntityManager;
struct FRecallArchetypeSnapshot;
struct FRecallArchetypeSaveCache;
struct FRecallEntityManagerSnapshot;

namespace Recall::EntitySnapshot::Save::Utils
{
	
extern void SaveEntityManager(const FMassEntityManager& EntityManager, int32 LastStaticEntitySerialNumber,
	FRecallEntityManagerSnapshot& NewSnapshot, TMap<FMassArchetypeHandle,
	FRecallArchetypeSaveCache>& ArchetypeCacheMap, TArray<FMassArchetypeHandle>& ArchetypeHandlesCache);
extern void SaveArchetypeSnapshot(const FMassEntityManager& EntityManager, int32 LastStaticEntitySerialNumber,
	FRecallArchetypeSnapshot& ArcheTypeSnapshot,
	const FMassArchetypeHandle& ArchetypeHandle, FRecallArchetypeSaveCache& ArchetypeCache);
	
} // namespace Recall::EntitySnapshot::Save::Utils
