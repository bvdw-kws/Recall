// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsCharacterProcessors.h"

#include "Async/ParallelFor.h"
#include "MassExecutionContext.h"
#include "Physics/Character/JPRPhysicsCharacterObject.h"
#include "Physics/Character/JPRPhysicsCharacterShapeTypes.h"
#include "Physics/Character/JPRPhysicsCharacterVirtualObject.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsCharacterFragments.h"
#include "Simulation/Physics/RecallPhysicsProcessorGroupTypes.h" 
#include "System/Physics/RecallPhysicsSubsystem.h"
#include "Utility/Physics/RecallPhysicsUtils.h"

//----------------------------------------------------------------------//
// URecallCharacterFragmentConstructor
//----------------------------------------------------------------------//
URecallCharacterFragmentConstructor::URecallCharacterFragmentConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FJPRPhysicsCharacterFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallCharacterFragmentConstructor::InitializeInternal(UObject& Owner,
	const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallCharacterFragmentConstructor::ConfigureQueries(
	const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJPRPhysicsCharacterShapeConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallCharacterFragmentConstructor::Execute(FMassEntityManager& EntityManager,
	FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_CharacterShape_Constructor);

	EntityQuery.ForEachEntityChunk(Context, ([](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const auto& Character = Context.GetConstSharedFragment<FJPRPhysicsCharacterShapeConstSharedFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, Character.Shape, Character.Params);

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, Character.Params);
		}
	}));
}

//----------------------------------------------------------------------//
// URecallPhysicsCharacterPostSimulationProcessor
//----------------------------------------------------------------------//
URecallPhysicsCharacterPostSimulationProcessor::URecallPhysicsCharacterPostSimulationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::EndPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::CharacterPostUpdate;
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::EndSimulation);
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::CharacterVirtualUpdate);
}

void URecallPhysicsCharacterPostSimulationProcessor::InitializeInternal(UObject& Owner,
	const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsCharacterPostSimulationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJPRPhysicsCharacterFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJPRPhysicsCharacterShapeConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsCharacterPostSimulationProcessor::Execute(FMassEntityManager& EntityManager,
	FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_CharacterPostSimulation_Execute);
	
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const auto& CharacterConstSharedFragment = Context.GetConstSharedFragment<FJPRPhysicsCharacterShapeConstSharedFragment>();
		if (!CharacterConstSharedFragment.Shape.bUseGroundState)
		{
			return;
		}
		
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TArrayView<FJPRPhysicsCharacterFragment> CharacterList = Context.GetMutableFragmentView<FJPRPhysicsCharacterFragment>();

		ParallelFor(Context.GetNumEntities(), [&PhysicsSystem, &BodyList, &CharacterList](int32 EntityIndex)
		{
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];

			const FRecallPhysicsBodyView BodyView = PhysicsSystem.GetMutableBody(BodyFragment.BodyHandle);
			const TWeakPtr<FJPRPhysicsCharacterBody> PhysicsBody = BodyView.GetBody<FJPRPhysicsCharacterBody>();
			if (!ensureMsgf(PhysicsBody.IsValid(), TEXT("Body does not exist.")))
			{
				return;
			}
			
			PhysicsBody.Pin()->PostSimulation();

			FJPRPhysicsCharacterFragment& CharacterFragment = CharacterList[EntityIndex];
			CharacterFragment.bIsSupported = PhysicsBody.Pin()->IsSupported();
			CharacterFragment.GroundState = PhysicsBody.Pin()->GetGroundState();
		});
	});
}

//----------------------------------------------------------------------//
// URecallCharacterVirtualFragmentConstructor
//----------------------------------------------------------------------//
URecallCharacterVirtualFragmentConstructor::URecallCharacterVirtualFragmentConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FJPRPhysicsCharacterFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallCharacterVirtualFragmentConstructor::InitializeInternal(UObject& Owner,
	const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallCharacterVirtualFragmentConstructor::ConfigureQueries(
	const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJPRPhysicsCharacterVirtualShapeConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallCharacterVirtualFragmentConstructor::Execute(FMassEntityManager& EntityManager,
	FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_CharacterVirtualShape_Constructor);

	EntityQuery.ForEachEntityChunk(Context, ([](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
		const auto& Character = Context.GetConstSharedFragment<FJPRPhysicsCharacterVirtualShapeConstSharedFragment>();

		const TArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetMutableFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			check(BodyFragment.BodyHandle.IsValid() == false);

			BodyFragment.BodyHandle = PhysicsSystem.CreateShape(Entity, Character.Shape, Character.Params);

			Recall::Physics::Utils::InitializePhysicsBody(Context, Entity,
				PhysicsSystem, BodyFragment, Character.Params);
		}
	}));
}

//----------------------------------------------------------------------//
// URecallPhysicsCharacterVirtualUpdateProcessor
//----------------------------------------------------------------------//
URecallPhysicsCharacterVirtualUpdateProcessor::URecallPhysicsCharacterVirtualUpdateProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::EndPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::CharacterVirtualUpdate;
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::EndSimulation);
}

void URecallPhysicsCharacterVirtualUpdateProcessor::InitializeInternal(UObject& Owner,
	const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsCharacterVirtualUpdateProcessor::ConfigureQueries(
	const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJPRPhysicsCharacterFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJPRPhysicsCharacterVirtualShapeConstSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsCharacterVirtualUpdateProcessor::Execute(FMassEntityManager& EntityManager,
	FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_CharacterVirtualUpdate_Execute);
	
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TArrayView<FJPRPhysicsCharacterFragment> CharacterList = Context.GetMutableFragmentView<FJPRPhysicsCharacterFragment>();

		const float DeltaTime = Context.GetDeltaTimeSeconds();
			
		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			FJPRPhysicsCharacterFragment& CharacterFragment = CharacterList[EntityIndex];

			const FRecallPhysicsBodyView BodyView = PhysicsSystem.GetMutableBody(BodyFragment.BodyHandle);
			const TWeakPtr<FJPRPhysicsCharacterVirtualBody> PhysicsBody = BodyView.GetBody<FJPRPhysicsCharacterVirtualBody>();
			if (!ensureMsgf(PhysicsBody.IsValid(), TEXT("Body does not exist.")))
			{
				continue;
			}

			PhysicsBody.Pin()->Update(DeltaTime);

			CharacterFragment.bIsSupported = PhysicsBody.Pin()->IsSupported();
			CharacterFragment.GroundState = PhysicsBody.Pin()->GetGroundState();
		}
	});
}
