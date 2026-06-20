// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "GameplayTagContainer.h"

#include "RecallGameplayTagTypes.generated.h"

USTRUCT()
struct RECALLSIMULATION_API FRecallGameplayTagCountMap
{
	GENERATED_BODY()

public:
	void AddTag(const FGameplayTag& Tag, int32 Count = 1);

	void RemoveTag(const FGameplayTag& Tag, int32 Count = 1);

	void ModifyTag(const FGameplayTag& Tag, int32 Modifier = 1);
	
	void AddTags(const FGameplayTagContainer& Container, int32 Count = 1);

	void RemoveTags(const FGameplayTagContainer& Container, int32 Count = 1);
	
	void SetTagCount(const FGameplayTag& Tag, int32 NewCount);
	
	int32 GetTagCount(const FGameplayTag& Tag) const;
	
	FGameplayTagContainer GetTags() const;
	
	bool HasTag(const FGameplayTag& Tag) const;
	
	bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;
	
	bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;

	const TMap<FGameplayTag, int32>& GetGameplayTagCountMap() const;
	
protected:
	/** Map of tag to active count of that tag */
	UPROPERTY(VisibleAnywhere)	
	TMap<FGameplayTag, int32> GameplayTagCountMap;
};
