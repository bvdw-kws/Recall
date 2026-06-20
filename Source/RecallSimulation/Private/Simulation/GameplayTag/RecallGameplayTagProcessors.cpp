// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallGameplayTagProcessors.h"

#include "MassExecutionContext.h"
#include "Actor/RecallActorGameplayTagInterface.h"
#include "Simulation/GameplayTag/RecallGameplayTagFragments.h"
#include "Simulation/Representation/RecallActorRepresentationFragments.h"
#include "System/Actor/RecallActorSubsystem.h"

//----------------------------------------------------------------------//
// URecallGameplayTagRepresentationProcessor
//----------------------------------------------------------------------//
URecallGameplayTagRepresentationProcessor::URecallGameplayTagRepresentationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::Render;
	bRequiresGameThreadExecution = true;
}

void URecallGameplayTagRepresentationProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallGameplayTagRepresentationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallActorRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallGameplayTagFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<URecallActorSubsystem>(EMassFragmentAccess::ReadOnly);
}

void URecallGameplayTagRepresentationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_GameplayTagRepresentation_Execute);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const URecallActorSubsystem& ActorSystem = Context.GetSubsystemChecked<URecallActorSubsystem>();

		const TConstArrayView<FRecallActorRepresentationFragment> ActorList = Context.GetFragmentView<FRecallActorRepresentationFragment>();
		const TConstArrayView<FRecallGameplayTagFragment> GameplayTagList = Context.GetFragmentView<FRecallGameplayTagFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallActorRepresentationFragment& ActorFragment = ActorList[EntityIndex];

			const TWeakObjectPtr<AActor> Actor = ActorSystem.GetActor<AActor>(ActorFragment.ActorHandle);
			if (!Actor.IsValid() || !Actor->GetClass()->ImplementsInterface(URecallActorGameplayTagInterface::StaticClass()))
			{
				continue;
			}

			const FRecallGameplayTagFragment& GameplayTagFragment = GameplayTagList[EntityIndex];

			IRecallActorGameplayTagInterface::Execute_OnRenderGameplayTags(
				Actor.Get(), GameplayTagFragment.GameplayTagCountMap.GetGameplayTagCountMap());
		}
	});
}
