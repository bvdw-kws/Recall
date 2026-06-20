// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsOverlapProcessors.h"

#include "MassExecutionContext.h"
#include "RecallSignalSubsystem.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h" 
#include "Simulation/Physics/RecallPhysicsProcessorGroupTypes.h"
#include "Simulation/Physics/RecallPhysicsSensorFragment.h"
#include "Simulation/Physics/RecallPhysicsSignalTypes.h"
#include "System/Physics/RecallPhysicsSubsystem.h"

//----------------------------------------------------------------------//
// URecallPhysicsOverlapProcessor
//----------------------------------------------------------------------//
URecallPhysicsOverlapProcessor::URecallPhysicsOverlapProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::Overlap;
}

void URecallPhysicsOverlapProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsOverlapProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsSensorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRecallPhysicsSensorTag>(EMassFragmentPresence::All);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<URecallSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsOverlapProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_Overlap);

	EntityQuery.ParallelForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const URecallPhysicsSubsystem& PhysicsSystem = Context.GetSubsystemChecked<URecallPhysicsSubsystem>();
		URecallSignalSubsystem& SignalSystem = Context.GetMutableSubsystemChecked<URecallSignalSubsystem>();

		const TArrayView<FRecallPhysicsSensorFragment> SensorList = Context.GetMutableFragmentView<FRecallPhysicsSensorFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			FRecallPhysicsSensorFragment& SensorFragment = SensorList[EntityIndex];

			const bool bWasAnyOverlapping = SensorFragment.IsAnySensorOverlapping();

			for (FRecallPhysicsSensorInstance& SensorInstance : SensorFragment.SensorInstances)
			{
				const TArray<FRecallPhysicsHitEvent> HitEvents = PhysicsSystem.GetHitEvents(SensorInstance.BodyHandle);
				if (!HitEvents.Num() && !SensorInstance.OverlappingEntities.Num())
				{
					continue;
				}

				const TSet<FMassEntityHandle> OldOverlappingEntities(SensorInstance.OverlappingEntities);

				SensorInstance.OverlappingEntities.Reset(HitEvents.Num());
				Algo::Transform(HitEvents, SensorInstance.OverlappingEntities,
					[](const FRecallPhysicsHitEvent& HitEvent)
				{
					return HitEvent.ColliderEntity;
				});

				// Was any new overlap detected?
				for (const FMassEntityHandle& NewOverlappingEntity : SensorInstance.OverlappingEntities)
				{
					// The new overlapping entity was already overlapping
					if (OldOverlappingEntities.Contains(NewOverlappingEntity))
					{
						continue;
					}

					SignalSystem.SignalEntity(Recall::Physics::Signals::NewOverlap, Entity);
					break;
				}

				// Was any old overlap lost?
				for (const FMassEntityHandle& OldOverlappingEntity : OldOverlappingEntities)
				{
					// The old overlapping entity is still overlapping
					if (SensorInstance.OverlappingEntities.Contains(OldOverlappingEntity))
					{
						continue;
					}
					
					SignalSystem.SignalEntity(Recall::Physics::Signals::MinusOverlap, Entity);
					break;
				}
			}

			const bool bIsAnyOverlapping = SensorFragment.IsAnySensorOverlapping();

			if (bWasAnyOverlapping != bIsAnyOverlapping)
			{
				if (bIsAnyOverlapping)
				{
					SignalSystem.SignalEntity(Recall::Physics::Signals::OverlapBegin, Entity);
					Context.Defer().AddTag<FRecallPhysicsSensorOverlapTag>(Entity);
				}
				else
				{
					SignalSystem.SignalEntity(Recall::Physics::Signals::OverlapEnd, Entity);
					Context.Defer().RemoveTag<FRecallPhysicsSensorOverlapTag>(Entity);
				}
			}
			else if (bIsAnyOverlapping)
			{
				SignalSystem.SignalEntity(Recall::Physics::Signals::OverlapUpdate, Entity);
			}
		}
	});
}
