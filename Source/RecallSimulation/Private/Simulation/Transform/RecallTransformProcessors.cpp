// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallTransformProcessors.h"

#include "MassExecutionContext.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Entity/RecallEntityAsyncSpawnSubsystem.h"
#include "Utility/Entity/RecallEntityUtils.h"

//----------------------------------------------------------------------//
// URecallTransformConstructor
//----------------------------------------------------------------------//
URecallTransformConstructor::URecallTransformConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FRecallTransformFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void URecallTransformConstructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallTransformConstructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) 
{
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<URecallEntityAsyncSpawnSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallTransformConstructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Transform_Constructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallEntityAsyncSpawnSubsystem& AsyncSpawnSystem = Context.GetMutableSubsystemChecked<URecallEntityAsyncSpawnSubsystem>();
		const FRecallEntityAsyncSpawnContext* SpawnContextPtr = AsyncSpawnSystem.PeekSpawnContext();
		if (SpawnContextPtr == nullptr)
		{
			return;
		}
			
		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);

			FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];
			TransformFragment.Position = SpawnContextPtr->Position;
			TransformFragment.Rotation = SpawnContextPtr->Rotation;

			Recall::Entity::Utils::AttachEntity(Context.GetEntityManagerChecked(),
				SpawnContextPtr->SpawnParameters.ParentEntity, Entity, TransformFragment);
		}
	});
}

//----------------------------------------------------------------------//
// URecallTransformDestructor
//----------------------------------------------------------------------//
URecallTransformDestructor::URecallTransformDestructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedTypes.Add(FRecallTransformFragment::StaticStruct());
	ObservedOperations = EMassObservedOperationFlags::Remove;
}

void URecallTransformDestructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallTransformDestructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) 
{
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
}

void URecallTransformDestructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Transform_Destructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			
			FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];

			Recall::Entity::Utils::DetachEntity(Context.GetEntityManagerChecked(), Entity, TransformFragment);
		}
	});
}
