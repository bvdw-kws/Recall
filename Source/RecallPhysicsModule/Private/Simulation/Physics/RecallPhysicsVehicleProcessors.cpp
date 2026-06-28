// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsVehicleProcessors.h"
#include "Simulation/Physics/RecallPhysicsProcessorGroupTypes.h"

#include "MassExecutionContext.h"
#include "Physics/Vehicle/RecallPhysicsVehicleObject.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h" 
#include "Simulation/Physics/RecallPhysicsVehicleFragments.h"
#include "System/Physics/RecallPhysicsSubsystem.h"
#include "Utility/Physics/RecallPhysicsUtils.h"

//----------------------------------------------------------------------//
// URecallVehicleShapeFragmentConstructor
//----------------------------------------------------------------------//
URecallVehicleShapeFragmentConstructor::URecallVehicleShapeFragmentConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FRecallPhysicsBodyFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallVehicleShapeFragmentConstructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallVehicleShapeFragmentConstructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FRecallPhysicsVehicleConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallVehicleShapeFragmentConstructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_VehicleShape_Constructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const FRecallPhysicsVehicleConstSharedFragment& VehicleShape = Context.GetConstSharedFragment<FRecallPhysicsVehicleConstSharedFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, VehicleShape.Shape, VehicleShape.Params);
			
			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, VehicleShape.Params);
		}
	});
}

//----------------------------------------------------------------------//
// URecallPhysicsVehicleProcessor
//----------------------------------------------------------------------//
URecallPhysicsVehicleProcessor::URecallPhysicsVehicleProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::StartPhysics;
	ExecutionOrder.ExecuteBefore.Add(Recall::Physics::ProcessorGroupNames::StartSimulation);
}

void URecallPhysicsVehicleProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsVehicleProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallPhysicsVehicleFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FRecallPhysicsVehicleConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsVehicleProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PhysicsVehicle_Execute);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		
		const FRecallPhysicsVehicleConstSharedFragment& VehicleConstSharedFragment = Context.GetConstSharedFragment<FRecallPhysicsVehicleConstSharedFragment>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TConstArrayView<FRecallPhysicsVehicleFragment> VehicleList = Context.GetFragmentView<FRecallPhysicsVehicleFragment>();
		
		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			const FRecallPhysicsVehicleFragment& VehicleFragment = VehicleList[EntityIndex];
			
			const FRecallPhysicsBodyView Body = PhysicsSystem.GetMutableBody(BodyFragment.BodyHandle);
			const TWeakPtr<FRecallPhysicsVehicleBody> Vehicle = StaticCastWeakPtr<FRecallPhysicsVehicleBody>(Body.GetBody());
			if (!Vehicle.IsValid())
			{
				continue;
			}

			Vehicle.Pin()->SetDriverInput(VehicleFragment.Forward, VehicleFragment.Right, VehicleFragment.Brake, VehicleFragment.HandBrake);
		}
	});
}
