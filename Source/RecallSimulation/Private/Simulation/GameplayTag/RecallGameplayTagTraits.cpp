// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/GameplayTag/RecallGameplayTagTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "Simulation/GameplayTag/RecallGameplayTagFragments.h"

void URecallGameplayTagsTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FRecallGameplayTagFragment& GameplayTagsFragment = BuildContext.AddFragment_GetRef<FRecallGameplayTagFragment>();

	TArray<FGameplayTag> AddTags;
	OwnedTags.GetGameplayTagArray(AddTags);

	for (const FGameplayTag& AddTag : AddTags)
	{
		GameplayTagsFragment.GameplayTagCountMap.AddTag(AddTag);
	}
	
	FRecallGameplayTagsConstSharedFragment ConstSharedFragment;
	ConstSharedFragment.NameTags = TSet<FName>(NameTags);

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(ConstSharedFragment));
	
	BuildContext.AddSharedFragment(EntityManager.GetOrCreateSharedFragment<FRecallGameplayTagGlobalSharedFragment>());
}
