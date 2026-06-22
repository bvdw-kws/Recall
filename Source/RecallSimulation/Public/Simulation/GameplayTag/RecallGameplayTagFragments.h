// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "GameplayTag/RecallGameplayTagTypes.h"

#include "RecallGameplayTagFragments.generated.h"

USTRUCT()
struct RECALLSIMULATION_API FRecallGameplayTagFragment : public FMassFragment
{
	GENERATED_BODY()

	/** Map of tag to active count of that tag */
	UPROPERTY(VisibleAnywhere)	
	FRecallGameplayTagCountMap GameplayTagCountMap;
};

template <>
struct TMassFragmentTraits<FRecallGameplayTagFragment> final
{ enum { AuthorAcceptsItsNotTriviallyCopyable = true }; };

USTRUCT()
struct RECALLSIMULATION_API FRecallGameplayTagsConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	// Static name tags attached to our entity
	UPROPERTY(VisibleAnywhere)
	TSet<FName> NameTags;

	FORCEINLINE bool HasAllNameTags(const TArray<FName>& RequiredNameTags) const
	{
		for (const FName& RequiredNameTag : RequiredNameTags)
		{
			if (!NameTags.Contains(RequiredNameTag))
			{
				return false;
			}
		}
		return true;
	};
};

/**
 * Global gameplay tags shared between all the entities.
 */
USTRUCT()
struct RECALLSIMULATION_API FRecallGameplayTagGlobalSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)	
	FRecallGameplayTagCountMap GameplayTagCountMap;
};
