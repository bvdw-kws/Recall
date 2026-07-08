// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsColliderProcessors.h"

#include "MassExecutionContext.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h" 
#include "Simulation/Physics/RecallPhysicsColliderFragments.h"
#include "Simulation/Physics/RecallPhysicsProcessorGroupTypes.h" 
#include "Simulation/Physics/RecallPhysicsSensorFragment.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Physics/RecallPhysicsSubsystem.h"
#include "Utility/Physics/RecallPhysicsUtils.h"

//----------------------------------------------------------------------//
// URecallBodyFragmentDestructor
//----------------------------------------------------------------------//
URecallBodyFragmentDestructor::URecallBodyFragmentDestructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FRecallPhysicsBodyFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Remove;
}

void URecallBodyFragmentDestructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallBodyFragmentDestructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallPhysicsSensorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallBodyFragmentDestructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PhysicsBody_Destructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();
		const TArrayView<FRecallPhysicsSensorFragment> SensorList = Context.GetMutableFragmentView<FRecallPhysicsSensorFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			FRecallPhysicsSensorFragment& SensorFragment = SensorList[EntityIndex];

			SensorFragment.RemoveSensor(BodyFragment.BodyHandle);
			PhysicsSystem.ReleaseBody(BodyFragment.BodyHandle);
		}
	});
}

//----------------------------------------------------------------------//
// URecallBoxShapeFragmentConstructor
//----------------------------------------------------------------------//
URecallBoxShapeFragmentConstructor::URecallBoxShapeFragmentConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FRecallPhysicsBodyFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallBoxShapeFragmentConstructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallBoxShapeFragmentConstructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJPRPhysicsBoxShapeFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallBoxShapeFragmentConstructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_BoxShape_Constructor);

	EntityQuery.ForEachEntityChunk(Context, ([](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const FJPRPhysicsBoxShapeFragment& BoxShape = Context.GetConstSharedFragment<FJPRPhysicsBoxShapeFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, BoxShape.Shape, BoxShape.Params);

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, BoxShape.Params);
		}
	}));
}

//----------------------------------------------------------------------//
// URecallSphereShapeFragmentConstructor
//----------------------------------------------------------------------//
URecallSphereShapeFragmentConstructor::URecallSphereShapeFragmentConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FRecallPhysicsBodyFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallSphereShapeFragmentConstructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallSphereShapeFragmentConstructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJPRPhysicsSphereShapeFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallSphereShapeFragmentConstructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SphereShape_Constructor);

	EntityQuery.ForEachEntityChunk(Context, ([](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const FJPRPhysicsSphereShapeFragment& SphereShape = Context.GetConstSharedFragment<FJPRPhysicsSphereShapeFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, SphereShape.Shape, SphereShape.Params);

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, SphereShape.Params);
		}
	}));
}

//----------------------------------------------------------------------//
// URecallCapsuleShapeFragmentConstructor
//----------------------------------------------------------------------//
URecallCapsuleShapeFragmentConstructor::URecallCapsuleShapeFragmentConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ObservedTypes.Add(FRecallPhysicsBodyFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallCapsuleShapeFragmentConstructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallCapsuleShapeFragmentConstructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FRecallPhysicsCapsuleFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallCapsuleShapeFragmentConstructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_CapsuleShape_Constructor);

	EntityQuery.ForEachEntityChunk(Context, ([](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const FRecallPhysicsCapsuleFragment& CapsuleShape = Context.GetConstSharedFragment<FRecallPhysicsCapsuleFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, CapsuleShape.Shape, CapsuleShape.Params);

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, CapsuleShape.Params);
		}
	}));
}

//----------------------------------------------------------------------//
// URecallMeshShapeFragmentProcessor
//----------------------------------------------------------------------//
URecallMeshShapeFragmentProcessor::URecallMeshShapeFragmentProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ProcessingPhase = EMassProcessingPhase::StartPhysics;
	ExecutionOrder.ExecuteBefore.Add(Recall::Physics::ProcessorGroupNames::Initialize);
}

void URecallMeshShapeFragmentProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallMeshShapeFragmentProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallPhysicsMeshFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRecallPhysicsBodyInitializedTransformTag>(EMassFragmentPresence::None);
	EntityQuery.AddConstSharedRequirement<FRecallPhysicsMeshConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallMeshShapeFragmentProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_MeshShape_Execute);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const FRecallPhysicsMeshConstSharedFragment& MeshConstSharedFragment = Context.GetConstSharedFragment<FRecallPhysicsMeshConstSharedFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();
		const TConstArrayView<FRecallPhysicsMeshFragment> MeshList = Context.GetFragmentView<FRecallPhysicsMeshFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			const FRecallPhysicsMeshFragment& MeshFragment = MeshList[EntityIndex];

			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			// Scale mesh
			const FTransform MeshTransform(FRotator::ZeroRotator, FVector::ZeroVector, MeshFragment.Scale);

			TArray<FVector3f> Vertices;
			Algo::Transform(MeshConstSharedFragment.Vertices, Vertices, [&MeshTransform](const FVector3f& Vertex)
				{
					return static_cast<FVector3f>(MeshTransform.TransformPosition(static_cast<FVector>(Vertex)));
				}
			);

			if (MeshConstSharedFragment.bConvexHull)
			{
				FJPRPhysicsConvexHullShape ConvexHullShape;
				ConvexHullShape.Vertices = Vertices;

				BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, ConvexHullShape, MeshConstSharedFragment.Params);
			}
			else
			{
				FJPRPhysicsMeshShape MeshShape;
				MeshShape.Vertices = Vertices;
				MeshShape.Triangles = MeshConstSharedFragment.Triangles;
				MeshShape.MeshShapeSettings = MeshConstSharedFragment.MeshShapeSettings;

				BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, MeshShape, MeshConstSharedFragment.Params);
			}

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, MeshConstSharedFragment.Params);
		}
	});
}

//----------------------------------------------------------------------//
// URecallHeightFieldShapeFragmentProcessor
//----------------------------------------------------------------------//
URecallHeightFieldShapeFragmentProcessor::URecallHeightFieldShapeFragmentProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::StartPhysics;
	ExecutionOrder.ExecuteBefore.Add(Recall::Physics::ProcessorGroupNames::Initialize);
}

void URecallHeightFieldShapeFragmentProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallHeightFieldShapeFragmentProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRecallPhysicsBodyInitializedTransformTag>(EMassFragmentPresence::None);
	EntityQuery.AddConstSharedRequirement<FRecallPhysicsHeightFieldConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallHeightFieldShapeFragmentProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_HeightField_Execute);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const FRecallPhysicsHeightFieldConstSharedFragment& HeightFieldConstSharedFragment = Context.GetConstSharedFragment<FRecallPhysicsHeightFieldConstSharedFragment>();
		if (HeightFieldConstSharedFragment.Shape.Heights.Num() == 0)
		{
			return;
		}
		
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		
		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();
		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);

			FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];
			TransformFragment.Position = HeightFieldConstSharedFragment.Location;
			TransformFragment.Rotation = HeightFieldConstSharedFragment.Rotation;
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity,
				HeightFieldConstSharedFragment.Shape, HeightFieldConstSharedFragment.Params);

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, HeightFieldConstSharedFragment.Params);
		}
	});
}
