// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Trait/RecallTraitUtils.h"

#include "Component/RecallEntityComponent.h"
#include "Serialization/ArchiveCrc32.h"
#include "StructUtils/StructView.h"

DEFINE_LOG_CATEGORY(LogRecallTrait);

namespace Recall::Trait::Utils
{

static uint32 MurmurFinalize32(uint32 Hash)
{
	Hash ^= Hash >> 16;
	Hash *= 0x85ebca6b;
	Hash ^= Hash >> 13;
	Hash *= 0xc2b2ae35;
	Hash ^= Hash >> 16;
	return Hash;
}

template<typename T>
static uint32 GetStructCrc32Helper(const T& Struct, const uint32 CRC)
{
	if (const UScriptStruct* ScriptStruct = Struct.GetScriptStruct())
	{
		return GetStructCrc32(*ScriptStruct, Struct.GetMemory(), CRC);
	}
	return 0;
}

uint32 GetStructCrc32(const UScriptStruct& ScriptStruct, const uint8* StructMemory, const uint32 CRC /*= 0*/)
{
	const FString StructPathName = ScriptStruct.GetPathName();
	const uint32 StructHash = GetTypeHash(StructPathName);

	FArchiveCrc32 Ar(HashCombine(CRC, MurmurFinalize32(StructHash)));
	if (StructMemory)
	{
		UScriptStruct& NonConstScriptStruct = const_cast<UScriptStruct&>(ScriptStruct);
		NonConstScriptStruct.SerializeItem(Ar, const_cast<uint8*>(StructMemory), nullptr);
	}
	return Ar.GetCrc();
}

uint32 GetStructCrc32(const FConstStructView& StructView, const uint32 CRC /*= 0*/)
{
	return GetStructCrc32Helper(StructView, CRC);
}
	
FMassEntityConfig* GetMutableEntityConfig(UObject* Outer)
{
	if (UMassEntityConfigAsset* EntityConfig = Cast<UMassEntityConfigAsset>(Outer))
	{
		return &EntityConfig->GetMutableConfig();
	}
	else if (URecallEntityComponent* EntityComponent = Cast<URecallEntityComponent>(Outer))
	{
		return &EntityComponent->GetMutableEntityConfig();
	}
	else
	{
		return nullptr;
	}
}

const FMassEntityConfig* GetEntityConfig(const UObject* Outer)
{
	return GetMutableEntityConfig(const_cast<UObject*>(Outer));
}
	
} // namespace Recall::Trait::Utils
