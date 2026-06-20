// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallDelayedSignalProcessor.h"

#include "MassExecutionContext.h"
#include "RecallSignalSubsystem.h"

//----------------------------------------------------------------------//
// URecallDelayedSignalProcessor
//----------------------------------------------------------------------//
URecallDelayedSignalProcessor::URecallDelayedSignalProcessor()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = Recall::Signals::ProcessorGroupNames::UpdateDelayedSignals;
}

void URecallDelayedSignalProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

bool URecallDelayedSignalProcessor::ShouldAllowQueryBasedPruning(const bool bRuntimeMode /*= true*/) const
{
	return false;
};

void URecallDelayedSignalProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallDelayedSignalProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_DelayedSignal_Execute);

	URecallSignalSubsystem& SignalsSystem = Context.GetMutableSubsystemChecked<URecallSignalSubsystem>();
	SignalsSystem.UpdateDelayedSignals(Context.GetDeltaTimeSeconds());
}
