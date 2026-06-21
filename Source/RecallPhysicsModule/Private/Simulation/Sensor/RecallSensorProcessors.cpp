// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSensorProcessors.h"

#include "Async/ParallelFor.h"
#include "MassExecutionContext.h"
#include "Physics/RecallPhysicsObjects.h"
#include "Simulation/Sensor/RecallSensorFragments.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsProcessorGroupTypes.h"
#include "Simulation/Physics/RecallPhysicsSensorFragment.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Physics/RecallPhysicsSubsystem.h"

//----------------------------------------------------------------------//
// URecallSensorDestructor
//----------------------------------------------------------------------//
URecallSensorDestructor::URecallSensorDestructor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ObservedTypes.Add(FRecallSensorFragment::StaticStruct());
	ObservedOperations.Add(EMassObservedOperation::Remove);
}

void URecallSensorDestructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallSensorDestructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallSensorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallPhysicsSensorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);

}

void URecallSensorDestructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Sensor_Destructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();

		const TArrayView<FRecallSensorFragment> SensorList = Context.GetMutableFragmentView<FRecallSensorFragment>();
		const TArrayView<FRecallPhysicsSensorFragment> SensorPhysicsList = Context.GetMutableFragmentView<FRecallPhysicsSensorFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			FRecallPhysicsSensorFragment& SensorPhysicsFragment = SensorPhysicsList[EntityIndex];
			FRecallSensorFragment& SensorFragment = SensorList[EntityIndex];

			for (FRecallPhysicsBodyHandle& BodyHandle : SensorFragment.BodyHandles)
			{
				SensorPhysicsFragment.RemoveSensor(BodyHandle);
				PhysicsSystem.ReleaseBody(BodyHandle);
			}
		}
	});
}

//----------------------------------------------------------------------//
// URecallSensorAttachmentProcessor
//----------------------------------------------------------------------//
URecallSensorAttachmentProcessor::URecallSensorAttachmentProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::StartPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::SensorAttachment;
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::Initialize);
}

void URecallSensorAttachmentProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallSensorAttachmentProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallSensorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FRecallSensorConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallSensorAttachmentProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Sensor_Attachment);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const FRecallSensorConstSharedFragment& SensorSharedFragment = Context.GetConstSharedFragment<FRecallSensorConstSharedFragment>();
		if (SensorSharedFragment.InstanceParameters.Num() == 0)
		{
			return;
		}

		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();

		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();
		const TConstArrayView<FRecallSensorFragment> SensorList = Context.GetFragmentView<FRecallSensorFragment>();

		ParallelFor(Context.GetNumEntities(), [&](int32 EntityIndex)
		{
			const FRecallSensorFragment& SensorFragment = SensorList[EntityIndex];

			for (int32 SensorIndex = 0; SensorIndex < SensorFragment.BodyHandles.Num(); SensorIndex++)
			{
				const FRecallPhysicsBodyHandle& SensorBodyHandle = SensorFragment.BodyHandles[SensorIndex];
				const TWeakPtr<FRecallPhysicsBody> SensorBody = PhysicsSystem.GetMutableBody(SensorBodyHandle);

				if (ensureMsgf(SensorBody.IsValid(), TEXT("Body does not exist.")) == false)
				{
					continue;
				}

				const FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];
				const FVector Position = TransformFragment.Position + TransformFragment.Rotation.RotateVector(SensorSharedFragment.InstanceParameters[SensorIndex].Offset);

				SensorBody.Pin()->SetPosition(Position);
				SensorBody.Pin()->Activate();
			}
		});
	});
}
