// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Rollback/RecallDebugRollbackManager.h"

#include "System/Simulation/Insight/RecallSimulationInsightSubsystem.h"
#include "Utility/Rollback/RecallRollbackUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void FRecallDebugRollbackManager::UpdateTimer(float FixedDeltaTime)
{
	RandomRollbackTimerHandle = FMath::Max(0.0f, RandomRollbackTimerHandle - FixedDeltaTime);
}

void FRecallDebugRollbackManager::ResetComparator()
{
	RollbackComparator.Reset();
}

bool FRecallDebugRollbackManager::ShouldApplyRandomRollback() const
{
	return Recall::Rollback::Utils::DebugUseRandomRollback() &&
		   FMath::IsNearlyZero(RandomRollbackTimerHandle) && FMath::RandBool();
}

void FRecallDebugRollbackManager::HandleRollbackComparison(uint32 Frame, uint32 LastSyncedFrame, URecallSimulationInsightSubsystem* InsightSystem)
{
	// Check if our state before and after this test rollback are the same
	if (RollbackComparator.IsValid() && InsightSystem)
	{
		const uint32 PreviousFrame = Frame - 1;
		const FRecallSimulationInsight NewComparator = InsightSystem->GenerateReportInRange(LastSyncedFrame, PreviousFrame);

		uint32 NewSyncedFrame = 0;
		if (!RollbackComparator->FindSyncedFrame(NewComparator, LastSyncedFrame, NewSyncedFrame) || NewSyncedFrame != PreviousFrame)
		{
			RollbackComparator->LogDiff(NewComparator, LastSyncedFrame);
		}

		ResetComparator();
	}
}

void FRecallDebugRollbackManager::SetupRollbackComparator(uint32 LastSyncedFrame, uint32 Frame, URecallSimulationInsightSubsystem* InsightSystem)
{
	if (Recall::Rollback::Utils::DebugUseRollbackComparator() && InsightSystem)
	{
		RollbackComparator = MakeShared<FRecallSimulationInsight>(
			InsightSystem->GenerateReportInRange(LastSyncedFrame, Frame - 1));
	}
}

void FRecallDebugRollbackManager::ApplyDebugOverrides(bool& bIsFrameSynced) const
{
	// Random rollback to simulate online conditions
	if (bIsFrameSynced && ShouldApplyRandomRollback())
	{
		RandomRollbackTimerHandle = Recall::Rollback::Utils::DebugRandomRollbackTimer();
		bIsFrameSynced = false;
	}
}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT