// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/RecallEntityStreamingSubsystem.h"

#include "Component/RecallEntityComponent.h"
#include "Fragments/RecallStreamingFragments.h"
#include "MassEntityManager.h"
#include "Settings/RecallSimulationSettings.h"
#include "Settings/RecallStreamingConfig.h"
#include "Simulation/RecallStreamingProcessors.h"
#include "System/Entity/RecallEntitySubsystem.h"

URecallEntityStreamingSubsystem::URecallEntityStreamingSubsystem()
{
	// The spatial hash grid is default-constructed
	// It will be reset and populated during Start()
}

void URecallEntityStreamingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallEntitySubsystem>();

	EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(GetWorld());
}

void URecallEntityStreamingSubsystem::Deinitialize()
{
	Super::Deinitialize();

	EntitySystem.Reset();
}

void URecallEntityStreamingSubsystem::Start(const FRecallSimulationStartParams& Params)
{
	// Initialize shared fragment for streaming state
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
	const FRecallStreamingStateFragment InitialState; // Default values
	EntityManager.GetOrCreateSharedFragment<FRecallStreamingStateFragment>(InitialState);
	
	// Collect all mutable static entity components from entity subsystem
	CollectMutableStaticEntities();
}

void URecallEntityStreamingSubsystem::Reset()
{
	// Clear all streaming state (only static data, temporary data is in shared fragments)
	MutableStaticComponents.Reset();
	
	// Reset the spatial hash grid
	StreamingGrid.Reset();
	
	// Reset streaming statistics
	StreamingStats = FRecallStreamingStats();
}

void URecallEntityStreamingSubsystem::CollectMutableStaticEntities()
{
	// Check global streaming setting
	const URecallSimulationSettings* SimSettings = GetDefault<URecallSimulationSettings>();
	const bool bGlobalStreamingEnabled = SimSettings && SimSettings->StreamingConfig.bEnableEntityStreaming;
	
	if (!bGlobalStreamingEnabled)
	{
		return; // Streaming disabled globally
	}
	
	const UWorld* World = GetWorld();
	if (!IsValid(World) || !EntitySystem.IsValid())
	{
		return;
	}
	
	// Access entity components from entity subsystem
	const TArray<URecallEntityComponent*>& EntityComponents = EntitySystem->GetEntityComponents();
	
	// Collect and sort by asset name for deterministic ordering
	TArray<TPair<FString, URecallEntityComponent*>> StreamableComponents;
	
	for (URecallEntityComponent* const Component : EntityComponents)
	{
		if (IsValid(Component) && Component->IsMutableEntity() && Component->IsStreamingEnabled())
		{
			const FString AssetName = Component->GetEntityAssetName();
			StreamableComponents.Add(TPair<FString, URecallEntityComponent*>(AssetName, Component));
		}
	}
	
	// Sort by asset name for deterministic behavior
	StreamableComponents.Sort([](const TPair<FString, URecallEntityComponent*>& A, const TPair<FString, URecallEntityComponent*>& B)
	{
		return A.Key < B.Key;
	});
	
	// Add to array in sorted order (rollback safe)
	MutableStaticComponents.Reserve(StreamableComponents.Num());
	for (const TTuple<FString, URecallEntityComponent*>& StreamablePair : StreamableComponents)
	{
		URecallEntityComponent* Component = StreamablePair.Value;
		if (!IsValid(Component))
		{
			continue;
		}

		const FMassEntityConfig& EntityConfig = Component->GetEntityConfig();
		const FMassEntityTemplate& EntityTemplate = EntityConfig.GetOrCreateEntityTemplate(*World);
		
		const FString& AssetName = StreamablePair.Key;
		
		MutableStaticComponents.Add(Component);
		
		// Add to spatial hash grid for efficient proximity queries
		if (const AActor* OwnerActor = Component->GetOwner())
		{
			FRecallStreamingGridItem GridItem;
			GridItem.Component = Component;
			GridItem.Location = OwnerActor->GetActorLocation();
			// Asset name removed from grid item - will get from component when needed
			GridItem.StreamingDistance = Component->GetStreamingDistance();
			GridItem.Priority = Component->GetStreamingPriority();
			
			// Use the component's custom streaming distance or default from config
			const float StreamDistance = GridItem.StreamingDistance > 0.0f ? 
				GridItem.StreamingDistance : GetStreamingConfig().LoadDistance;
			
			// Validate streaming distance
			if (StreamDistance <= 0.0f || !FMath::IsFinite(StreamDistance))
			{
				UE_LOG(LogRecallEntityStreaming, Warning,
					TEXT("%hs Invalid streaming distance %f for entity %s, skipping"), 
					__FUNCTION__, StreamDistance, *AssetName);
				continue;
			}
			
			// Create bounds for the grid based on streaming distance
			// Use 2D bounds (ignore Z) for more efficient spatial queries
			const FBox Bounds(
				GridItem.Location - FVector(StreamDistance, StreamDistance, 0.0f),
				GridItem.Location + FVector(StreamDistance, StreamDistance, 0.0f)
			);
			
			// Validate bounds before adding
			if (!Bounds.IsValid)
			{
				UE_LOG(LogRecallEntityStreaming, Warning, TEXT("Invalid bounds for entity %s at location %s with distance %f"), 
					*AssetName, *GridItem.Location.ToString(), StreamDistance);
				continue;
			}
			
			StreamingGrid.Add(GridItem, Bounds);
		}
	}

	// Update statistics
	StreamingStats.TotalStreamableEntities = MutableStaticComponents.Num();
}

FMassEntityHandle URecallEntityStreamingSubsystem::SpawnStreamableEntity(URecallEntityComponent* EntityComponent) const
{
	if (!EntityComponent || !EntitySystem.IsValid())
	{
		return FMassEntityHandle();
	}
	
	// Create the entity using the entity subsystem's streaming method
	const FMassEntityHandle Entity = EntitySystem->CreateStreamingEntity(EntityComponent);
	
	if (Entity.IsValid())
	{
		// Add the streaming fragment to mark this entity as streamable
		FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
		
		const FString& AssetName = EntityComponent->GetEntityAssetName();
		
		EntityManager.AddFragmentToEntity(Entity, FRecallStreamableEntityFragment::StaticStruct(),
			[EntityComponent, &AssetName](void* FragmentMemory, const UScriptStruct& FragmentType)
			{
				FRecallStreamableEntityFragment& StreamingFragment = *static_cast<FRecallStreamableEntityFragment*>(FragmentMemory);
				StreamingFragment.EntityAssetName = AssetName;
				StreamingFragment.SourceComponent = *EntityComponent->GetEntityAssetName();
			}
		);
		
		// Update statistics
		StreamingStats.LoadedEntities++;
		
		UE_LOG(LogRecallEntityStreaming, Log,
			TEXT("%hs Spawned streaming entity: %s"), __FUNCTION__, *AssetName);
	}
	
	return Entity;
}

void URecallEntityStreamingSubsystem::QueryStreamingGridItems(
	const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations,
	TArray<FRecallStreamingGridItem>& NearbyItems,
	TSet<FName>& FilterComponents, float Range, int32 MaxCount) const
{
	const FRecallStreamingConfig& Config = GetStreamingConfig();
	
	// Process entities using spatial queries
	int32 EntitiesProcessed = 0;
	
	// Query the hash grid around each view position
	for (int32 ViewIndex = 0; ViewIndex < ViewPositions.Num(); ++ViewIndex)
	{
		if (EntitiesProcessed >= MaxCount)
		{
			break; // Limit processing per frame
		}
		
		const FVector& ViewPosition = ViewPositions[ViewIndex];
		const FQuat& ViewRotation = ViewRotations[ViewIndex];
		
		// Create query bounds around the view position
		const FBox QueryBounds(
			ViewPosition - FVector(Range, Range, 0.0),
			ViewPosition + FVector(Range, Range, 0.0)
		);
		
		// Process each level of the hierarchical grid
		for (int32 Level = 0; Level < StreamingGrid.NumLevels; Level++)
		{
			const FRecallStreamingHashGrid2D::FCellRect Rect = StreamingGrid.CalcQueryBounds(QueryBounds, Level);
		
			for (int32 Y = Rect.MinY; Y <= Rect.MaxY; Y++)
			{
				for (int32 X = Rect.MinX; X <= Rect.MaxX; X++)
				{
					const FRecallStreamingHashGrid2D::FCell* Cell = StreamingGrid.FindCell(X, Y, Level);
					if (Cell == nullptr)
					{
						continue;
					}

					const TSparseArray<FRecallStreamingHashGrid2D::FItem>& Items = StreamingGrid.GetItems();
					for (int32 Idx = Cell->First; Idx != INDEX_NONE; Idx = Items[Idx].Next)
					{
						const FRecallStreamingGridItem& GridItem = Items[Idx].ID;
					
						// Skip if the component is invalid
						const URecallEntityComponent* Component = GridItem.Component.Get();
						if (!IsValid(Component))
						{
							continue;
						}
						
						// Skip if the component is already loaded
						const FName ComponentName = *Component->GetEntityAssetName();
						if (FilterComponents.Contains(ComponentName))
						{
							continue;
						}
					
						// Check distance
						const float DistSquared = FVector::DistSquared(GridItem.Location, ViewPosition);
						const float StreamDistance = GridItem.StreamingDistance > 0.0f ? 
							GridItem.StreamingDistance : Range;

						if (DistSquared > StreamDistance * StreamDistance)
						{
							continue;
						}
					
						// Check visibility if enabled
						if (!IsEntityVisible(GridItem.Location, ViewPositions, ViewRotations, Config))
						{
							continue;
						}

						FilterComponents.Add(ComponentName);
						NearbyItems.Add(GridItem);
						
						EntitiesProcessed++;
						if (EntitiesProcessed >= MaxCount)
						{
							return;
						}
					}
				}
			}
		}
	}
}

bool URecallEntityStreamingSubsystem::IsEntityVisible(const FVector& EntityPosition, const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations, const FRecallStreamingConfig& Config) const
{
	if (!Config.bEnableVisibilityStreaming)
	{
		return true; // No visibility filtering
	}
	
	const float CosHalfFOV = FMath::Cos(FMath::DegreesToRadians(Config.VisibilityFOV * 0.5f));
	
	for (int32 ViewIndex = 0; ViewIndex < ViewPositions.Num(); ++ViewIndex)
	{
		const FVector& ViewPosition = ViewPositions[ViewIndex];
		const FQuat& ViewRotation = ViewRotations[ViewIndex];
		
		const FVector ToEntity = EntityPosition - ViewPosition;
		const float Distance = ToEntity.Size();
		
		// Check if entity is behind view target within back buffer distance
		const FVector ViewForward = ViewRotation.GetForwardVector();
		const float DotProduct = FVector::DotProduct(ToEntity.GetSafeNormal(), ViewForward);
		
		if (DotProduct < 0.0f && FMath::Abs(DotProduct * Distance) <= Config.VisibilityBackBuffer)
		{
			return true; // Within back buffer
		}
		
		// Check if entity is within FOV cone
		if (DotProduct >= CosHalfFOV)
		{
			return true; // Within view cone
		}
	}
	
	return false; // Not visible from any view target
}

const FRecallStreamingConfig& URecallEntityStreamingSubsystem::GetStreamingConfig() const
{
	const URecallSimulationSettings* SimSettings = GetDefault<URecallSimulationSettings>();
	check(SimSettings);
	return SimSettings->StreamingConfig;
}
