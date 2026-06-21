// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallEntitySnapshotTypes.h"

#include "MassEntityManager.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

template<typename T>
static void WriteBitSet(T& Bitset, TArray<uint8>& Data)
{
	FMemoryWriter ArWriter(Data);
	Bitset.Serialize(ArWriter);
}

template<typename T>
static void ReadBitSet(T& Bitset, const TArray<uint8>& Data)
{
	FMemoryReader ArReader(Data);
	Bitset.Serialize(ArReader);
}

void FRecallSharedFragmentValuesSnapshot::Set(const FMassEntityManager& EntityManager, const FMassArchetypeSharedFragmentValues& SharedFragmentValues)
{
	FMassSharedFragmentBitSet SharedFragmentBitSet;

	const TMap<uint32, FSharedStruct> HashToSharedFragmentsMap = EntityManager.GetSharedFragmentValuesMap();

	for (const FSharedStruct& SharedFragment : SharedFragmentValues.GetSharedFragments())
	{
		const uint32 Hash = EntityManager.GetSharedFragmentHashChecked(SharedFragment);
		SharedFragmentsHashes.Add(Hash);
	}

	for (const FConstSharedStruct& ConstSharedFragment : SharedFragmentValues.GetConstSharedFragments())
	{
		const uint32 Hash = EntityManager.GetConstSharedFragmentHashChecked(ConstSharedFragment);
		ConstSharedFragmentsHashes.Add(Hash);
	}
}

FMassArchetypeSharedFragmentValues FRecallSharedFragmentValuesSnapshot::Get(const FMassEntityManager& EntityManager) const
{
	FMassArchetypeSharedFragmentValues SharedFragmentValues;

	for (const uint32 SharedFragmentsHash : SharedFragmentsHashes)
	{
		const FSharedStruct& SharedFragment = EntityManager.GetSharedFragmentByHashChecked(SharedFragmentsHash);
		SharedFragmentValues.Add(SharedFragment);
	}

	for (const uint32 ConstSharedFragmentsHash : ConstSharedFragmentsHashes)
	{
		const FConstSharedStruct& ConstSharedFragment = EntityManager.GetConstSharedFragmentByHashChecked(ConstSharedFragmentsHash);
		SharedFragmentValues.Add(ConstSharedFragment);
	}

	SharedFragmentValues.Sort();
	return SharedFragmentValues;
}

void FRecallArchetypeCompositionSnapshot::Set(const FMassArchetypeCompositionDescriptor& Composition)
{
	const FMassFragmentBitSet& Fragments = Composition.GetFragments();
	for (auto It = Fragments.GetIndexIterator(); It; ++It)
	{
		FragmentTypes.Add(Fragments.GetTypeAtIndex(*It));
	}
	const FMassTagBitSet& Tags = Composition.GetTags();
	for (auto It = Tags.GetIndexIterator(); It; ++It)
	{
		TagTypes.Add(Tags.GetTypeAtIndex(*It));
	}
	const FMassChunkFragmentBitSet& ChunkFragments = Composition.GetChunkFragments();
	for (auto It = ChunkFragments.GetIndexIterator(); It; ++It)
	{
		ChunkFragmentTypes.Add(ChunkFragments.GetTypeAtIndex(*It));
	}
	const FMassSharedFragmentBitSet& SharedFragments = Composition.GetSharedFragments();
	for (auto It = SharedFragments.GetIndexIterator(); It; ++It)
	{
		SharedFragmentTypes.Add(SharedFragments.GetTypeAtIndex(*It));
	}
	const FMassConstSharedFragmentBitSet& ConstSharedFragments = Composition.GetConstSharedFragments();
	for (auto It = ConstSharedFragments.GetIndexIterator(); It; ++It)
	{
		ConstSharedFragmentTypes.Add(ConstSharedFragments.GetTypeAtIndex(*It));
	}
}

FMassArchetypeCompositionDescriptor FRecallArchetypeCompositionSnapshot::Get() const
{
	FMassArchetypeCompositionDescriptor Composition;

	for (const auto& FragmentType : FragmentTypes)
	{
		Composition.Add(FragmentType);
	}
	for (const auto& TagType : TagTypes)
	{
		Composition.Add(TagType);
	}
	for (const auto& ChunkFragmentType : ChunkFragmentTypes)
	{
		Composition.Add(ChunkFragmentType);
	}
	for (const auto& SharedFragmentType : SharedFragmentTypes)
	{
		Composition.Add(SharedFragmentType);
	}
	for (const auto& ConstSharedFragmentType : ConstSharedFragmentTypes)
	{
		Composition.Add(ConstSharedFragmentType);
	}

	return Composition;
}

void FRecallEntityManagerSnapshot::SetSharedFragmentsMap(const TMap<uint32, FSharedStruct>& InSharedFragmentsMap)
{
	SharedFragmentsMap.Empty(InSharedFragmentsMap.Num());

	for (const auto& SharedFragment : InSharedFragmentsMap)
	{
		const UScriptStruct* Type = SharedFragment.Value.GetScriptStruct();
		FInstancedStruct& Instance = SharedFragmentsMap.Add(SharedFragment.Key);
		Instance.InitializeAs(Type, SharedFragment.Value.GetMemory());
	}
}
