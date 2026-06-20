// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Representation/RecallActorRepresentationTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "MassEntityView.h"
#include "Actor/RecallSkeletalMeshActorRepresentationInterface.h"
#include "Simulation/Representation/RecallActorRepresentationFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "Utility/Trait/RecallTraitUtils.h"

//----------------------------------------------------------------------//
// URecallActorRepresentationTraitBase
//----------------------------------------------------------------------//
void URecallActorRepresentationTraitBase::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
	const UWorld& World) const
{
	BuildContext.RequireFragment<FRecallTransformFragment>();

	FRecallActorRepresentationFragment& ActorFragment = BuildContext.AddFragment_GetRef<FRecallActorRepresentationFragment>();
	ActorFragment.Offset = Offset;
	ActorFragment.Scale = Scale;
	
	if (bUseActorScale)
	{
		BuildContext.GetMutableObjectFragmentInitializers().Add([this, &World](
				UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
			{
				if (const AActor* Actor = Recall::Trait::Utils::AsActor(Owner))
				{
					FRecallActorRepresentationFragment& ActorRepresentationFragment = EntityView.GetFragmentData<FRecallActorRepresentationFragment>();
					ActorRepresentationFragment.Scale *= Actor->GetActorScale3D();
				}
			}
		);
	}
}

//----------------------------------------------------------------------//
// URecallSkeletalMeshActorRepresentationTrait
//----------------------------------------------------------------------//
void URecallSkeletalMeshActorRepresentationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FRecallSkeletalMeshActorRepresentationConstSharedFragment SharedFragment;
	SharedFragment.Definition = Mesh;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));

	BuildContext.AddTag<FRecallSkeletalMeshActorRepresentationTag>();
}

void URecallSkeletalMeshActorRepresentationTrait::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (Mesh.MeshScale_DEPRECATED != FVector::OneVector)
	{
		Scale = Mesh.MeshScale_DEPRECATED;
		Mesh.MeshScale_DEPRECATED = FVector::OneVector;
	}
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------//
// URecallStaticMeshActorRepresentationTrait
//----------------------------------------------------------------------//
void URecallStaticMeshActorRepresentationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FRecallStaticMeshActorRepresentationConstSharedFragment SharedFragment;
	SharedFragment.Definition = Mesh;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));
}

void URecallStaticMeshActorRepresentationTrait::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (Mesh.MeshScale_DEPRECATED != FVector::OneVector)
	{
		Scale = Mesh.MeshScale_DEPRECATED;
		Mesh.MeshScale_DEPRECATED = FVector::OneVector;
	}
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------//
// URecallActorRepresentationTrait
//----------------------------------------------------------------------//
void URecallActorRepresentationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FRecallActorRepresentationConstSharedFragment SharedFragment;
	SharedFragment.Definition = Actor;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));

	if (bIsSkeletalMeshActor)
	{
		BuildContext.AddTag<FRecallSkeletalMeshActorRepresentationTag>();
	}
}

void URecallActorRepresentationTrait::PostLoad()
{
	Super::PostLoad();

	bIsSkeletalMeshActor = false;

	if (UClass* ActorClass = Actor.Blueprint.TryLoadClass<AActor>())
	{
		bIsSkeletalMeshActor = ActorClass->ImplementsInterface(URecallSkeletalMeshActorRepresentationInterface::StaticClass());
	}

#if WITH_EDITOR
	if (Actor.MeshScale_DEPRECATED != FVector::OneVector)
	{
		Scale = Actor.MeshScale_DEPRECATED;
		Actor.MeshScale_DEPRECATED = FVector::OneVector;
	}
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------//
// URecallDecalRepresentationTrait
//----------------------------------------------------------------------//
void URecallDecalRepresentationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FRecallDecalActorRepresentationConstSharedFragment ConstSharedFragment;
	ConstSharedFragment.Desc = DecalActor;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(ConstSharedFragment));
}

void URecallDecalRepresentationTrait::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (DecalActor.MeshScale_DEPRECATED != FVector::OneVector)
	{
		Scale = DecalActor.MeshScale_DEPRECATED;
		DecalActor.MeshScale_DEPRECATED = FVector::OneVector;
	}
#endif // WITH_EDITOR
}
