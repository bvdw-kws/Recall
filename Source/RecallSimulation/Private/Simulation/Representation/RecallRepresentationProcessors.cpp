// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallRepresentationProcessors.h"

#include "MassExecutionContext.h"
#include "Observer/Representation/RecallRepresentationReactInterface.h"
#include "System/Observer/RecallObserverSubjectSubsystem.h"

//----------------------------------------------------------------------//
// URecallRepresentationProcessor
//----------------------------------------------------------------------//
URecallRepresentationProcessor::URecallRepresentationProcessor()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::Render;
}

void URecallRepresentationProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

bool URecallRepresentationProcessor::ShouldAllowQueryBasedPruning(const bool bRuntimeMode) const
{
	return false;
}

void URecallRepresentationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallObserverSubjectSubsystem>(EMassFragmentAccess::ReadOnly);
}

void URecallRepresentationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Representation_Execute);

	const URecallObserverSubjectSubsystem& ObserverSubjectSystem = Context.GetSubsystemChecked<URecallObserverSubjectSubsystem>();
	const auto Observers = ObserverSubjectSystem.GetObservers<IRecallRepresentationReactInterface>();

	/**
	 * This callback is done while we are rendering the game simulation, and the simulation thread is not stepping.
	 * This way entities and other simulation data can be safely read.
	 */
	for (const auto& Observer : Observers)
	{
		Observer.Interface.OnRender();
	}
}
