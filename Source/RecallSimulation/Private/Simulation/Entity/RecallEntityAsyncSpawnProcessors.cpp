// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallEntityAsyncSpawnProcessors.h"

#include "MassExecutionContext.h"
#include "System/Entity/RecallEntityAsyncSpawnSubsystem.h"

//----------------------------------------------------------------------//
// URecallEntityAsyncSpawnProcessor
//----------------------------------------------------------------------//
URecallEntityAsyncSpawnProcessor::URecallEntityAsyncSpawnProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void URecallEntityAsyncSpawnProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

bool URecallEntityAsyncSpawnProcessor::ShouldAllowQueryBasedPruning(const bool bRuntimeMode) const
{
	return false;
}

void URecallEntityAsyncSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallEntityAsyncSpawnSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallEntityAsyncSpawnProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_EntityAsyncSpawn_Execute);

	URecallEntityAsyncSpawnSubsystem& AsyncSpawnSystem = Context.GetMutableSubsystemChecked<URecallEntityAsyncSpawnSubsystem>();
	if (AsyncSpawnSystem.IsAnyRequestReady() == false)
	{
		return;
	}

	Context.Defer().PushCommand<FMassDeferredCreateCommand>([&AsyncSpawnSystem](FMassEntityManager& System)
	{
		AsyncSpawnSystem.PushQueue(System);
	});
}
