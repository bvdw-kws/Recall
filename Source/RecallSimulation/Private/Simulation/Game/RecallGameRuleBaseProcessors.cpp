// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallGameRuleBaseProcessors.h"

#include "MassExecutionContext.h"
#include "System/Game/RecallGameRuleSubsystem.h"

//----------------------------------------------------------------------//
// URecallGameRuleBaseProcessor
//----------------------------------------------------------------------//
URecallGameRuleBaseProcessor::URecallGameRuleBaseProcessor()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
}

void URecallGameRuleBaseProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

bool URecallGameRuleBaseProcessor::ShouldAllowQueryBasedPruning(const bool bRuntimeMode) const
{
	return false;
}

void URecallGameRuleBaseProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallGameRuleSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallGameRuleBaseProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_GameRuleBase_Execute);
	
	URecallGameRuleSubsystem& GameRuleSystem = Context.GetMutableSubsystemChecked<URecallGameRuleSubsystem>();
	GameRuleSystem.TickMatchTime(Context.GetDeltaTimeSeconds());
}
