// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Entity/RecallEntityUtils.h"

#include "MassEntityManager.h"
#include "MassEntityView.h"
#include "MassEntityUtils.h"
#include "RecallSignalSubsystem.h"
#include "Simulation/Controller/RecallControllerFragments.h"
#include "Simulation/GameplayTag/RecallGameplayTagFragments.h"
#include "Simulation/Representation/RecallActorRepresentationFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "Simulation/Transform/RecallTransformSignalTypes.h"
#include "System/Actor/RecallActorSubsystem.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "Utility/Player/RecallPlayerUtils.h"

namespace Recall::Entity::Utils
{
	
TArray<FMassEntityHandle> GetAllMutableEntities(const UWorld* World)
{
	TArray<FMassEntityHandle> Results;
	
	if (!IsValid(World))
	{
		return Results;
	}

	const URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World);
	if (!IsValid(EntitySystem))
	{
		return Results;
	}
 	
	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	const TArray<FMassEntityHandle> Entities = EntityManager.GetAllEntities();
	const int32 StaticEntityCount = EntitySystem->GetStaticEntityCount();
	check(StaticEntityCount >= 0);

	int32 MutableEntityCount = 0;
	if (Entities.Num())
	{
		MutableEntityCount = Entities.Num() - StaticEntityCount;
		check(MutableEntityCount >= 0);
	}

	Results.SetNum(MutableEntityCount);

	for (int32 EntityIndex = 0; EntityIndex < MutableEntityCount; ++EntityIndex)
	{
		Results[EntityIndex] = Entities[EntityIndex + StaticEntityCount];
	}

	return Results;
}
	
TArray<FMassEntityHandle> FilterEntitiesByTag(
	const UWorld* World, const TArray<FMassEntityHandle>& Entities, const FGameplayTagContainer& GameplayTags, TOptional<TArray<FName>> NameTags,
	int32 MaxEntityCount)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_EntityUtils_FilterEntitiesByTag);
 	
	TArray<FMassEntityHandle> Results;
	if (!IsValid(World) || MaxEntityCount <= 0)
	{
		return Results;	
	}

	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);

	TArray<bool> bRequirements;
	bRequirements.SetNum(Entities.Num());

	ParallelFor(Entities.Num(),
		[&bRequirements, &EntityManager, &Entities, &GameplayTags, &NameTags](int32 Index)
	{
		const FMassEntityHandle& Entity = Entities[Index];
		const FMassEntityView EntityView(EntityManager, Entity);

		// Gameplay tag
		const FRecallGameplayTagFragment* GameplayTagsFragmentPtr = EntityView.GetFragmentDataPtr<FRecallGameplayTagFragment>();
		if (GameplayTagsFragmentPtr == nullptr || !GameplayTagsFragmentPtr->GameplayTagCountMap.HasAllMatchingGameplayTags(GameplayTags))
		{
			return;
		}

		// Optional name tag
		if (NameTags.IsSet())
		{
			const auto& ConstSharedFragment = EntityView.GetConstSharedFragmentData<FRecallGameplayTagsConstSharedFragment>();
			if (!ConstSharedFragment.HasAllNameTags(NameTags.GetValue()))
			{
				return;
			}
		}
		
		bRequirements[Index] = true;
	});
	
	for (int32 EntityIndex = 0; EntityIndex < Entities.Num(); EntityIndex++)
	{
		if (!bRequirements[EntityIndex])
		{
			continue;
		}
		
		const FMassEntityHandle& Entity = Entities[EntityIndex];
		
		Results.Add(Entity);
		if (Results.Num() >= MaxEntityCount)
		{
			break;
		}
	}

	return Results;
}
	
 TArray<FMassEntityHandle> GetAllEntitiesByTag(const UWorld* World, const FGameplayTagContainer& GameplayTags,
 	TOptional<TArray<FName>> NameTags, int32 MaxEntityCount)
 {
	QUICK_SCOPE_CYCLE_COUNTER(Recall_EntityUtils_GetAllEntitiesByTag);
 	
 	TArray<FMassEntityHandle> Results;
 	if (!IsValid(World))
 	{
 		return Results;	
 	}

 	const TArray<FMassEntityHandle> MutableEntities = GetAllMutableEntities(World);

	Results = FilterEntitiesByTag(World, MutableEntities, GameplayTags, NameTags, MaxEntityCount);

	return Results;
 }
	
void AttachEntity(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ParentEntity, const FMassEntityHandle& ChildrenEntity, bool bSignal /*= true*/)
{
	if (!EntityManager.IsEntityValid(ChildrenEntity))
	{
		return;
	}

	const FMassEntityView ChildrenView(EntityManager, ChildrenEntity);
	FRecallTransformFragment* ChildrenTransformFragmentPtr = ChildrenView.GetFragmentDataPtr<FRecallTransformFragment>();
	if (ChildrenTransformFragmentPtr == nullptr)
	{
		return;
	}

	AttachEntity(EntityManager, ParentEntity, ChildrenEntity, *ChildrenTransformFragmentPtr, bSignal);
}
	
void AttachEntity(const FMassEntityManager& EntityManager, const FMassEntityHandle& ParentEntity,
	const FMassEntityHandle& ChildrenEntity, FRecallTransformFragment& ChildrenTransformFragment, bool bSignal /*= true*/)
{
	if (!ParentEntity.IsSet() || !EntityManager.IsEntityValid(ParentEntity))
	{
		return;
	}

	const FMassEntityView ParentView(EntityManager, ParentEntity);
	FRecallTransformFragment* ParentTransformFragmentPtr = ParentView.GetFragmentDataPtr<FRecallTransformFragment>();
	if (ParentTransformFragmentPtr == nullptr)
	{
		return;
	}

	AttachEntity(EntityManager, ParentEntity, *ParentTransformFragmentPtr,
		ChildrenEntity, ChildrenTransformFragment, bSignal);
}
	
void AttachEntity(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ParentEntity, FRecallTransformFragment& ParentTransformFragment,
	const FMassEntityHandle& ChildrenEntity, FRecallTransformFragment& ChildrenTransformFragment, bool bSignal /*= true*/)
{
	if (ChildrenTransformFragment.AttachParent == ParentEntity ||
		ParentTransformFragment.AttachChildren.Contains(ChildrenEntity))
	{
		check(ChildrenTransformFragment.AttachParent == ParentEntity);
		check(ParentTransformFragment.AttachChildren.Contains(ChildrenEntity));
		return;
	}
	
	if (ChildrenTransformFragment.AttachParent.IsSet())
	{
		DetachEntity(EntityManager, ChildrenEntity);
	}
	
	ChildrenTransformFragment.AttachParent = ParentEntity;
	ParentTransformFragment.AttachChildren.Add(ChildrenEntity);
	
	if (bSignal)
	{
		if (URecallSignalSubsystem* SignalSystem = UWorld::GetSubsystem<URecallSignalSubsystem>(EntityManager.GetWorld()))
		{
			SignalSystem->SignalEntity(Recall::Transform::Signals::AttachChildren, ParentEntity);
		}
	}
}
	
void DetachEntity(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ChildrenEntity, bool bSignal /*= true*/)
{
	if (!EntityManager.IsEntityValid(ChildrenEntity))
	{
		return;
	}

	const FMassEntityView ChildrenView(EntityManager, ChildrenEntity);
	FRecallTransformFragment* ChildrenTransformFragmentPtr = ChildrenView.GetFragmentDataPtr<FRecallTransformFragment>();
	if (ChildrenTransformFragmentPtr == nullptr)
	{
		return;
	}
	
	DetachEntity(EntityManager, ChildrenEntity, *ChildrenTransformFragmentPtr, bSignal);
}
	
void DetachEntity(
	const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ChildrenEntity, FRecallTransformFragment& ChildrenTransformFragment, bool bSignal /*= true*/)
{
	if (!ChildrenTransformFragment.AttachParent.IsSet())
	{
		return;
	}

	if (!EntityManager.IsEntityValid(ChildrenTransformFragment.AttachParent))
	{
		ChildrenTransformFragment.AttachParent.Reset();
		return;
	}

	if (bSignal)
	{
		if (URecallSignalSubsystem* SignalSystem = UWorld::GetSubsystem<URecallSignalSubsystem>(EntityManager.GetWorld()))
		{
			SignalSystem->SignalEntity(Recall::Transform::Signals::DetachChildren, ChildrenTransformFragment.AttachParent);
		}
	}
	
	const FMassEntityView ParentView(EntityManager, ChildrenTransformFragment.AttachParent);
	FRecallTransformFragment& ParentTransformFragment = ParentView.GetFragmentData<FRecallTransformFragment>();

	check(ParentTransformFragment.AttachChildren.Contains(ChildrenEntity));

	ChildrenTransformFragment.AttachParent.Reset();
	ParentTransformFragment.AttachChildren.Remove(ChildrenEntity);
}

FTransform GetEntityTransform(const FMassEntityManager& EntityManager, const FMassEntityHandle& Entity)
{
	if (!EntityManager.IsEntityValid(Entity))
	{
		return FTransform::Identity;
	}

	const FMassEntityView EntityView(EntityManager, Entity);
	if (const auto* TransformFragmentPtr = EntityView.GetFragmentDataPtr<FRecallTransformFragment>())
	{
		return TransformFragmentPtr->GetTransform();
	}

	return FTransform::Identity;
}

FRecallActorHandle GetControllerCameraActorHandle(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ControllerEntity)
{
	if (!EntityManager.IsEntityValid(ControllerEntity))
	{
		return FRecallActorHandle();
	}

	const FMassEntityView ControllerView(EntityManager, ControllerEntity);

	if (auto* ControllerFragmentPtr = ControllerView.GetFragmentDataPtr<FRecallControllerFragment>())
	{
		if (EntityManager.IsEntityValid(ControllerFragmentPtr->CameraEntity))
		{
			const FMassEntityView CameraView(EntityManager, ControllerFragmentPtr->CameraEntity);
			if (const auto* CameraActorFragmentPtr = CameraView.GetFragmentDataPtr<FRecallActorRepresentationFragment>())
			{
				return CameraActorFragmentPtr->ActorHandle;
			}
		}
	}

	return FRecallActorHandle();
}

TWeakObjectPtr<AActor> GetControllerViewTarget(const UWorld* World, const FString& ControllerID)
{
	const URecallActorSubsystem* ActorSystem = UWorld::GetSubsystem<URecallActorSubsystem>(World);
	if (!IsValid(ActorSystem))
	{
		return nullptr;
	}
	
	FMassEntityHandle ControllerEntity;
	if (!Player::Utils::FindPlayerEntityInWorld(World, ControllerID, ControllerEntity))
	{
		return nullptr;
	}
	
	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	const FMassEntityView ControllerView(EntityManager, ControllerEntity);

	const FRecallControllerFragment* ControllerFragmentPtr = ControllerView.GetFragmentDataPtr<FRecallControllerFragment>();
	if (ControllerFragmentPtr != nullptr && EntityManager.IsEntityValid(ControllerFragmentPtr->ViewTargetEntity))
	{
		const FMassEntityView ViewTargetView(EntityManager, ControllerFragmentPtr->ViewTargetEntity);
		if (const auto* ViewTargetFragmentPtr = ViewTargetView.GetFragmentDataPtr<FRecallActorRepresentationFragment>())
		{
			const TWeakObjectPtr<AActor> ViewTarget = ActorSystem->GetActor<AActor>(ViewTargetFragmentPtr->ActorHandle);
			if (ViewTarget.IsValid())
			{
				return ViewTarget;
			}
		}
	}

	if (const FRecallActorRepresentationFragment* ControllerActorFragmentPtr = ControllerView.GetFragmentDataPtr<FRecallActorRepresentationFragment>())
	{
		return ActorSystem->GetActor<AActor>(ControllerActorFragmentPtr->ActorHandle);
	}

	return nullptr;
}

TWeakObjectPtr<AActor> GetControllerPawn(const UWorld* World, const FString& ControllerID)
{
	FMassEntityHandle EntityHandle;
	if (!Player::Utils::FindPlayerEntityInWorld(World, ControllerID, EntityHandle))
	{
		return nullptr;
	}
	
	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	const FMassEntityView EntityView(EntityManager, EntityHandle);

	const URecallActorSubsystem* ActorSystemPtr = UWorld::GetSubsystem<URecallActorSubsystem>(World);
	const FRecallActorRepresentationFragment* ActorFragmentPtr = EntityView.GetFragmentDataPtr<FRecallActorRepresentationFragment>();
	if (ActorSystemPtr == nullptr || ActorFragmentPtr == nullptr)
	{
		return nullptr;
	}

	return ActorSystemPtr->GetActor<AActor>(ActorFragmentPtr->ActorHandle);
}
	
} // namespace Recall::Entity::Utils
