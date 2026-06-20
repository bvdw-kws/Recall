// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassProcessor.h"

#include "RecallStreamingProcessors.generated.h"

struct FRecallStreamingConfig;

DECLARE_LOG_CATEGORY_EXTERN(LogRecallEntityStreaming, Log, All);

/**
 * Processor that handles view target collection and streaming decisions
 */
UCLASS()
class URecallEntityStreamingProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	URecallEntityStreamingProcessor();
	
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	/** Query for controller entities with view targets */
	FMassEntityQuery ControllerQuery;
	
	/** Query for currently loaded streamable entities */
	FMassEntityQuery StreamableEntityQuery;

	/** Cache manager for temporary per-frame data to avoid repeated allocations */
	TSharedPtr<struct FRecallEntityStreamingCacheManager> CacheManager;
	
	/** Collect view target positions and rotations from controller entities */
	void CollectViewTargets(FMassExecutionContext& Context, TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations);
	
	/** Process streaming decisions for unloaded entities */
	void ProcessStreamingDecisions(FMassExecutionContext& Context,
		const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations, const FRecallStreamingConfig& Config);
	
	/** Process despawn decisions for loaded entities */
	void ProcessDespawnDecisions(FMassExecutionContext& Context, const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations);
	
	/** Check if streaming should run this frame based on update interval */
	bool ShouldProcessStreaming(FMassExecutionContext& Context, const FRecallStreamingConfig& Config);
};