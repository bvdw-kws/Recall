// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FMassEntityHandle;
struct FMassEntityManager;
struct FRecallActorHandle;
struct FRecallTransformFragment;
struct FGameplayTagContainer;

namespace Recall::Entity::Utils
{

/**
 * Return a list of all non-static entities.
 */
RECALLSIMULATION_API extern TArray<FMassEntityHandle> GetAllMutableEntities(const UWorld* World);
	
/**
 * Filter entities per tags. Only iterate through mutable entities.
 */
RECALLSIMULATION_API extern TArray<FMassEntityHandle> GetAllEntitiesByTag(
	const UWorld* World, const FGameplayTagContainer& GameplayTags, TOptional<TArray<FName>> NameTags = TOptional<TArray<FName>>(),
	int32 MaxEntityCount = INT32_MAX);
RECALLSIMULATION_API extern TArray<FMassEntityHandle> FilterEntitiesByTag(
	const UWorld* World, const TArray<FMassEntityHandle>& Entities, const FGameplayTagContainer& GameplayTags,
	TOptional<TArray<FName>> NameTags = TOptional<TArray<FName>>(), int32 MaxEntityCount = INT32_MAX);
	
/**
 * Attach children entity to parent entity.
 */
RECALLSIMULATION_API extern void AttachEntity(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ParentEntity, const FMassEntityHandle& ChildrenEntity, bool bSignal = true);
RECALLSIMULATION_API extern void AttachEntity(
	const FMassEntityManager& EntityManager, const FMassEntityHandle& ParentEntity,
	const FMassEntityHandle& ChildrenEntity, FRecallTransformFragment& ChildrenTransformFragment, bool bSignal = true);
RECALLSIMULATION_API extern void AttachEntity(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ParentEntity, FRecallTransformFragment& ParentTransformFragment,
	const FMassEntityHandle& ChildrenEntity, FRecallTransformFragment& ChildrenTransformFragment, bool bSignal = true);
	
RECALLSIMULATION_API extern void DetachEntity(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ChildrenEntity, bool bSignal = true);
RECALLSIMULATION_API extern void DetachEntity(
	const FMassEntityManager& EntityManager,
	const FMassEntityHandle& ChildrenEntity, FRecallTransformFragment& ChildrenTransformFragment, bool bSignal = true);

RECALLSIMULATION_API extern FTransform GetEntityTransform(const FMassEntityManager& EntityManager,
	const FMassEntityHandle& Entity);

RECALLSIMULATION_API extern FRecallActorHandle GetControllerCameraActorHandle(
	const FMassEntityManager& EntityManager, const FMassEntityHandle& ControllerEntity);
	
// Controller
RECALLSIMULATION_API extern TWeakObjectPtr<AActor> GetControllerViewTarget(const UWorld* World, const FString& ControllerID);	
RECALLSIMULATION_API extern TWeakObjectPtr<AActor> GetControllerPawn(const UWorld* World, const FString& ControllerID);	
	
} // namespace Recall::Entity::Utils
