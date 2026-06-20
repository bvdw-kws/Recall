// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "System/RecallStreamingTypes.h"

#include "RecallEntityStreamingSubsystem.generated.h"

class URecallEntityComponent;
struct FMassEntityHandle;
struct FRecallStreamingConfig;

/**
 * Streaming statistics structure
 */
USTRUCT()
struct RECALLENTITYSTREAMING_API FRecallStreamingStats
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Stats")
	int32 LoadedEntities = 0;

	UPROPERTY(VisibleAnywhere, Category="Stats")
	int32 UnloadedEntities = 0;

	UPROPERTY(VisibleAnywhere, Category="Stats")
	int32 TotalStreamableEntities = 0;

	UPROPERTY(VisibleAnywhere, Category="Stats")
	float MemorySaved = 0.0f; // Estimated MB
};

/**
 * Manages entity streaming by controlling spawn/despawn of mutable static entities
 * based on view target proximity
 */
UCLASS()
class RECALLENTITYSTREAMING_API URecallEntityStreamingSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	URecallEntityStreamingSubsystem();
	
	/** Collect all mutable static entity components from entity subsystem and populate spatial hash grid */
	void CollectMutableStaticEntities();

	/** Spawn streamable entity from component and add streaming fragment */
	FMassEntityHandle SpawnStreamableEntity(URecallEntityComponent* EntityComponent) const;
	
	/** Get all mutable static components for processor access */
	const TArray<TWeakObjectPtr<URecallEntityComponent>>& GetMutableStaticComponents() const { return MutableStaticComponents; }
	
	/** Query streaming grid for nearby entities within range of view positions */
	void QueryStreamingGridItems(const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations,
		TArray<FRecallStreamingGridItem>& NearbyItems, TSet<FName>& FilterComponents,
		float Range = 5000.0f, int32 MaxCount = 10) const;
	
	/** Check if entity is visible from view targets (approximate) */
	bool IsEntityVisible(const FVector& EntityPosition, const TArray<FVector>& ViewPositions, const TArray<FQuat>& ViewRotations,
		const FRecallStreamingConfig& Config) const;
	
	/** Update streaming statistics when entity is despawned */
	void UpdateStatsOnDespawn() const { StreamingStats.LoadedEntities--; StreamingStats.UnloadedEntities++; }

protected:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IRecallSimulationReactSystemInterface implementation
	virtual void Start(const FRecallSimulationStartParams& Params) override;
	virtual int32 GetStartOrderPriority() const override { return Recall::SimReactSystem::StartOrder::LowPriority; }
	virtual void Reset() override;
	
protected:
	/** Static array of mutable static entity components collected during Start() - rollback safe */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<URecallEntityComponent>> MutableStaticComponents;
	/** Entity subsystem to spawn streaming entities */
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallEntitySubsystem> EntitySystem;

	/** Streaming statistics - mutable for tracking purposes only */
	UPROPERTY(Transient)
	mutable FRecallStreamingStats StreamingStats;
	
	/** Spatial hash grid for efficient proximity queries - not serialized, rebuilt on Start */
	FRecallStreamingHashGrid2D StreamingGrid;

private:
	/** Get streaming configuration from simulation settings */
	const struct FRecallStreamingConfig& GetStreamingConfig() const;
};

template<>
struct TMassExternalSubsystemTraits<URecallEntityStreamingSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
