// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FRecallSimulationInsight;

/**
 * Debug utilities for rollback system development and testing
 */
struct RECALLONLINE_API FRecallDebugRollbackManager
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	mutable float RandomRollbackTimerHandle = 0.0f;
	TSharedPtr<FRecallSimulationInsight> RollbackComparator;
	
	/** Updates the random rollback timer with fixed delta time */
	void UpdateTimer(float FixedDeltaTime);
	
	/** Resets the rollback comparator */
	void ResetComparator();
	
	/** Checks if random rollback should be applied based on debug settings */
	bool ShouldApplyRandomRollback() const;
	
	/** Handles debug rollback comparison validation */
	void HandleRollbackComparison(uint32 Frame, uint32 LastSyncedFrame, class URecallSimulationInsightSubsystem* InsightSystem);
	
	/** Sets up rollback comparator for debug validation */
	void SetupRollbackComparator(uint32 LastSyncedFrame, uint32 Frame, class URecallSimulationInsightSubsystem* InsightSystem);
	
	/** Applies debug overrides to frame sync state including random rollback */
	void ApplyDebugOverrides(bool& bIsFrameSynced) const;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
};