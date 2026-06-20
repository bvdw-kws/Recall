// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallStreamingProcessors.h"

#include "MassExecutionContext.h"
#include "Component/RecallEntityComponent.h"
#include "Fragments/RecallStreamingFragments.h"
#include "Settings/RecallSimulationSettings.h"
#include "Settings/RecallStreamingConfig.h"
#include "Simulation/Controller/RecallControllerFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/RecallEntityStreamingSubsystem.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

DEFINE_LOG_CATEGORY(LogRecallEntityStreaming);

//----------------------------------------------------------------------//
// URecallEntityStreamingProcessor
//----------------------------------------------------------------------//
URecallEntityStreamingProcessor::URecallEntityStreamingProcessor()
	: ControllerQuery(*this)
	, StreamableEntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

/**
 * Cache manager for URecallEntityStreamingProcessor to store temporary per-frame data
 * and avoid repeated memory allocations during streaming calculations
 */
struct FRecallEntityStreamingCacheManager
{
	/** Cached view target positions collected from controller entities */
	TArray<FVector> ViewPositions;
	
	/** Cached view target rotations for visibility calculations */
	TArray<FQuat> ViewRotations;

	/** Set of already loaded components to prevent duplicate spawning */
	TSet<FName> LoadedComponents;
	
	/** Cached results from spatial hash grid queries */
	TArray<FRecallStreamingGridItem> NearbyItems;
	
	/** Reset all cached data for the next frame while preserving allocated memory */
	FORCEINLINE void ResetCache()
	{
		ViewPositions.Reset();
		ViewRotations.Reset();

		LoadedComponents.Reset();
		NearbyItems.Reset();
	}
};

void URecallEntityStreamingProcessor::InitializeInternal(UObject& Owner,
	const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);

	CacheManager = MakeShared<FRecallEntityStreamingCacheManager>();
}

void URecallEntityStreamingProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// Query for controller entities with view targets (position and rotation)
	ControllerQuery.AddRequirement<FRecallControllerFragment>(EMassFragmentAccess::ReadOnly);
	
	// Query for currently loaded streamable entities
	StreamableEntityQuery.AddRequirement<FRecallStreamableEntityFragment>(EMassFragmentAccess::ReadOnly);
	StreamableEntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadOnly);
	StreamableEntityQuery.AddSubsystemRequirement<URecallEntityStreamingSubsystem>(EMassFragmentAccess::ReadOnly);
	
	// Require streaming subsystem access
	ProcessorRequirements.AddSubsystemRequirement<URecallEntityStreamingSubsystem>(EMassFragmentAccess::ReadOnly);
}

void URecallEntityStreamingProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	check(CacheManager.IsValid());
	CacheManager->ResetCache();
	
	const URecallSimulationSettings* SimSettings = GetDefault<URecallSimulationSettings>();
	const FRecallStreamingConfig& Config = SimSettings->StreamingConfig;
	
	// Check if we should process streaming this frame using shared fragment
	if (!ShouldProcessStreaming(Context, Config))
	{
		return;
	}

	// Update shared fragment with current time/frame
	const double TimeSeconds = Recall::Simulation::Utils::GetTimeSeconds(Context.GetWorld());
	EntityManager.ForEachSharedFragment<FRecallStreamingStateFragment>(
		[TimeSeconds](FRecallStreamingStateFragment& StreamingState)
		{
			StreamingState.LastStreamingUpdateTime = TimeSeconds; // Use simulation time
		}
	);
	
	// Collect view targets from controller entities
	TArray<FVector>& ViewPositions = CacheManager->ViewPositions;
	TArray<FQuat>& ViewRotations = CacheManager->ViewRotations;
	CollectViewTargets(Context, ViewPositions, ViewRotations);
	
	// Process streaming decisions
	ProcessStreamingDecisions(Context, ViewPositions, ViewRotations, Config);
	
	// Process despawn decisions for currently loaded entities
	ProcessDespawnDecisions(Context, ViewPositions, ViewRotations);
}

bool URecallEntityStreamingProcessor::ShouldProcessStreaming(FMassExecutionContext& Context, const FRecallStreamingConfig& Config)
{
	const URecallSimulationSettings* SimSettings = GetDefault<URecallSimulationSettings>();
	if (!SimSettings->StreamingConfig.bEnableEntityStreaming)
	{
		return false; // Streaming disabled globally
	}
	
	// Use shared fragment for deterministic timing
	FMassEntityManager& EntityManager = Context.GetEntityManagerChecked();
	double LastStreamingUpdateTime = 0.0f;
	EntityManager.ForEachSharedFragment<FRecallStreamingStateFragment>(
		[&LastStreamingUpdateTime](FRecallStreamingStateFragment& StreamingStateFragment)
		{
			LastStreamingUpdateTime = StreamingStateFragment.LastStreamingUpdateTime;
		}
	);
	const double CurrentTime = Recall::Simulation::Utils::GetTimeSeconds(Context.GetWorld()); // Simulation time
	
	return static_cast<float>(CurrentTime - LastStreamingUpdateTime) >= Config.UpdateInterval;
}

void URecallEntityStreamingProcessor::CollectViewTargets(FMassExecutionContext& Context,
	TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations)
{
	ControllerQuery.ForEachEntityChunk(Context, [&OutPositions, &OutRotations](FMassExecutionContext& Context)
	{
		const FMassEntityManager& EntityManager = Context.GetEntityManagerChecked();
		
		const TConstArrayView<FRecallControllerFragment> ControllerList = Context.GetFragmentView<FRecallControllerFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallControllerFragment& Controller = ControllerList[EntityIndex];
			
			// Get transform from view target entity instead of controller
			if (Controller.ViewTargetEntity.IsValid() && EntityManager.IsEntityValid(Controller.ViewTargetEntity))
			{
				const FMassEntityView ViewTargetEntityView(EntityManager, Controller.ViewTargetEntity);
				if (const FRecallTransformFragment* ViewTargetTransform = ViewTargetEntityView.GetFragmentDataPtr<FRecallTransformFragment>())
				{
					OutPositions.Add(ViewTargetTransform->Position);
					OutRotations.Add(ViewTargetTransform->Rotation);
				}
			}
		}
	});
}

void URecallEntityStreamingProcessor::ProcessStreamingDecisions(FMassExecutionContext& Context,
	const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations, const FRecallStreamingConfig& Config)
{
	const URecallEntityStreamingSubsystem& StreamingSystem = Context.GetSubsystemChecked<URecallEntityStreamingSubsystem>();
	
	// Track which components are already loaded to avoid duplicates
	TSet<FName>& LoadedComponents = CacheManager->LoadedComponents;
	
	// Build set of already loaded components from streamable entity query
	StreamableEntityQuery.ForEachEntityChunk(Context, [&LoadedComponents](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FRecallStreamableEntityFragment> StreamableList = ChunkContext.GetFragmentView<FRecallStreamableEntityFragment>();
		
		// Track loaded components by their pointer
		for (int32 EntityIndex = 0; EntityIndex < ChunkContext.GetNumEntities(); EntityIndex++)
		{
			const FRecallStreamableEntityFragment& StreamableFragment = StreamableList[EntityIndex];
			if (!StreamableFragment.SourceComponent.IsNone())
			{
				LoadedComponents.Add(StreamableFragment.SourceComponent);
			}
		}
	});

	// Query the spatial hash grid
	TArray<FRecallStreamingGridItem>& NearbyItems = CacheManager->NearbyItems;
	StreamingSystem.QueryStreamingGridItems(ViewPositions, ViewRotations,
		NearbyItems, LoadedComponents, Config.LoadDistance, Config.MaxEntitiesPerFrame);
	
	// Spawn the entity
	for (const FRecallStreamingGridItem& NearbyItem : NearbyItems)
	{
		const TWeakObjectPtr<URecallEntityComponent> WeakComponent(NearbyItem.Component);
		Context.Defer().PushCommand<FMassDeferredCreateCommand>(
			[WeakComponent, &StreamingSystem](FMassEntityManager& System)
			{
				if (!WeakComponent.IsValid())
				{
					return;
				}
				
				StreamingSystem.SpawnStreamableEntity(WeakComponent.Get());
			}
		);
	}
}

void URecallEntityStreamingProcessor::ProcessDespawnDecisions(FMassExecutionContext& Context,
	const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations)
{
	// Process loaded streamable entities for potential despawning
	StreamableEntityQuery.ForEachEntityChunk(Context,
		[&ViewPositions, &ViewRotations](FMassExecutionContext& Context)
	{
		const URecallSimulationSettings* SimSettings = GetDefault<URecallSimulationSettings>();
		const FRecallStreamingConfig& Config = SimSettings->StreamingConfig;
		const float UnloadDistSquared = Config.UnloadDistance * Config.UnloadDistance;
			
		const URecallEntityStreamingSubsystem& StreamingSystem = Context.GetSubsystemChecked<URecallEntityStreamingSubsystem>();
		
		const TConstArrayView<FRecallStreamableEntityFragment> StreamableList = Context.GetFragmentView<FRecallStreamableEntityFragment>();
		const TConstArrayView<FRecallTransformFragment> TransformList = Context.GetFragmentView<FRecallTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			const FRecallStreamableEntityFragment& StreamableFragment = StreamableList[EntityIndex];
			const FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];
			
			// Check if entity should be unloaded
			bool bShouldUnload = true;
			for (int32 ViewIndex = 0; ViewIndex < ViewPositions.Num(); ++ViewIndex)
			{
				const float DistSquared = FVector::DistSquared(TransformFragment.Position, ViewPositions[ViewIndex]);
				if (DistSquared > UnloadDistSquared)
				{
					continue;
				}
				
				// Check visibility if enabled
				if (StreamingSystem.IsEntityVisible(TransformFragment.Position, ViewPositions, ViewRotations, Config))
				{
					bShouldUnload = false;
					break;
				}
			}
			
			// Despawn the entity
			if (bShouldUnload)
			{				
				// Destroy the entity - this will clean up all fragments and physics bodies
				Context.Defer().DestroyEntity(Entity);
				
				// Update statistics in the subsystem
				StreamingSystem.UpdateStatsOnDespawn();
				
				UE_LOG(LogRecallEntityStreaming, Log,
					TEXT("%hs Despawned streaming entity: %s"), __FUNCTION__, *StreamableFragment.EntityAssetName);
			}
		}
	});
}
