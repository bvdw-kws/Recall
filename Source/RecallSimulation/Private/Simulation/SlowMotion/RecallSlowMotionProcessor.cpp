// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSlowMotionProcessor.h"

#include "MassExecutionContext.h"
#include "System/Slowmo/RecallSlowMotionSubsystem.h"

//----------------------------------------------------------------------//
// URecallSlowMotionProcessor
//----------------------------------------------------------------------//
URecallSlowMotionProcessor::URecallSlowMotionProcessor()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);

	// Update time dilatation at the start of the frame
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void URecallSlowMotionProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

bool URecallSlowMotionProcessor::ShouldAllowQueryBasedPruning(const bool bRuntimeMode /*= true*/) const
{
	return false;
};

void URecallSlowMotionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallSlowMotionSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallSlowMotionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SlowMotion_Execute);

	URecallSlowMotionSubsystem& SlowMotionSystem = Context.GetMutableSubsystemChecked<URecallSlowMotionSubsystem>();
	SlowMotionSystem.UpdateTimeDilatation();
}
