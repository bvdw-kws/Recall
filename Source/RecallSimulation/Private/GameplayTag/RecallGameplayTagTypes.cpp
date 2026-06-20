// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "GameplayTag/RecallGameplayTagTypes.h"

void FRecallGameplayTagCountMap::AddTag(const FGameplayTag& Tag, int32 Count)
{
	GameplayTagCountMap.FindOrAdd(Tag) += FMath::Max(0, Count);
}

void FRecallGameplayTagCountMap::RemoveTag(const FGameplayTag& Tag, int32 Count)
{
	if (int32* CountPtr = GameplayTagCountMap.Find(Tag))
	{
		int32& CountRef = *CountPtr;
		CountRef = FMath::Max(0, CountRef - Count);
		if (CountRef <= 0)
		{
			// Remove from map so that we do not replicate
			GameplayTagCountMap.Remove(Tag);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FMinimalReplicationTagCountMap::RemoveTag called on Tag %s that wasn't in the tag map."), *Tag.ToString());
	}
}

void FRecallGameplayTagCountMap::ModifyTag(const FGameplayTag& Tag, int32 Modifier)
{
	if (Modifier > 0)
	{
		AddTag(Tag, Modifier);
	}
	else if (Modifier < 0)
	{
		RemoveTag(Tag, -Modifier);
	}
}

void FRecallGameplayTagCountMap::AddTags(const FGameplayTagContainer& Container, int32 Count)
{
	for (const FGameplayTag& Tag : Container)
	{
		AddTag(Tag, Count);
	}
}

void FRecallGameplayTagCountMap::RemoveTags(const FGameplayTagContainer& Container, int32 Count)
{
	for (const FGameplayTag& Tag : Container)
	{
		RemoveTag(Tag, Count);
	}
}

void FRecallGameplayTagCountMap::SetTagCount(const FGameplayTag& Tag, int32 NewCount)
{
	int32& Count = GameplayTagCountMap.FindOrAdd(Tag);
	Count = FMath::Max(0, NewCount);
	
	if (Count == 0)
	{
		// Remove from map so that we do not replicate
		GameplayTagCountMap.Remove(Tag);
	}
}

int32 FRecallGameplayTagCountMap::GetTagCount(const FGameplayTag& Tag) const
{
	if (const int32* Ptr = GameplayTagCountMap.Find(Tag))
	{
		return *Ptr;
	}

	return 0;
}

FGameplayTagContainer FRecallGameplayTagCountMap::GetTags() const
{
	FGameplayTagContainer ItemTags;

	for (const TPair<FGameplayTag, int32>& GameplayTagCount : GameplayTagCountMap)
	{
		if (GameplayTagCount.Value > 0)
		{
			ItemTags.AddTag(GameplayTagCount.Key);
		}
	}

	return ItemTags;
}

bool FRecallGameplayTagCountMap::HasTag(const FGameplayTag& Tag) const
{
	return GetTagCount(Tag) > 0;
}

bool FRecallGameplayTagCountMap::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	// if the TagContainer count is 0 return bCountEmptyAsMatch;
	if (TagContainer.Num() == 0)
	{
		return true;
	}

	bool AllMatch = true;
	for (const FGameplayTag& Tag : TagContainer)
	{
		if (GameplayTagCountMap.FindRef(Tag) <= 0)
		{
			AllMatch = false;
			break;
		}
	}		
	return AllMatch;
}

bool FRecallGameplayTagCountMap::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (TagContainer.Num() == 0)
	{
		return false;
	}

	bool AnyMatch = false;
	for (const FGameplayTag& Tag : TagContainer)
	{
		if (GameplayTagCountMap.FindRef(Tag) > 0)
		{
			AnyMatch = true;
			break;
		}
	}
	return AnyMatch;
}

const TMap<FGameplayTag, int32>& FRecallGameplayTagCountMap::GetGameplayTagCountMap() const
{
	return GameplayTagCountMap;
}