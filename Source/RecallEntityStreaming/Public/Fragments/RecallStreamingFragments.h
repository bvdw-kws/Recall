// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassEntityView.h"

#include "RecallStreamingFragments.generated.h"

/**
 * Marks entities that are currently loaded and streamable
 * Only exists on loaded entities - absence indicates entity is unloaded
 */
USTRUCT()
struct RECALLENTITYSTREAMING_API FRecallStreamableEntityFragment : public FMassFragment
{
	GENERATED_BODY()

	/** Asset name of the source entity component for respawning */
	UPROPERTY(VisibleAnywhere)
	FString EntityAssetName;
	
	/** Pointer to the source component for duplicate checking */
	UPROPERTY(VisibleAnywhere)
	FName SourceComponent = NAME_Name;
};

/**
 * Shared fragment to store streaming state that must be deterministic and rollback-safe
 */
USTRUCT()
struct RECALLENTITYSTREAMING_API FRecallStreamingStateFragment : public FMassSharedFragment
{
	GENERATED_BODY()

	/** Last time streaming was processed (simulation time) */
	UPROPERTY(VisibleAnywhere)
	double LastStreamingUpdateTime = 0.0;
};