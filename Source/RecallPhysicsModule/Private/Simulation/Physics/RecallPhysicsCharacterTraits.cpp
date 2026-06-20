// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Physics/RecallPhysicsCharacterTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "MassEntityUtils.h"
#include "Physics/Character/RecallPhysicsCharacterShapeTypes.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsCharacterFragments.h"

//----------------------------------------------------------------------//
// URecallCharacterCollisionTrait
//----------------------------------------------------------------------//
void URecallCharacterCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
	const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.AddFragment<FRecallPhysicsCharacterFragment>();

	FRecallPhysicsCharacterShapeConstSharedFragment CharacterFragment;
	CharacterFragment.Shape = CharacterShape;
	CharacterFragment.Params = Params;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(CharacterFragment));
}

FVector URecallCharacterCollisionTrait::GetExtents() const
{
	return FVector(
		CharacterShape.Capsule.Radius,
		CharacterShape.Capsule.Radius,
		CharacterShape.Capsule.HalfHeightOfCylinder + CharacterShape.Capsule.Radius);
}

//----------------------------------------------------------------------//
// URecallCharacterVirtualCollisionTrait
//----------------------------------------------------------------------//
void URecallCharacterVirtualCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
	const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	BuildContext.AddFragment<FRecallPhysicsCharacterFragment>();
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FRecallPhysicsCharacterVirtualShapeConstSharedFragment CharacterFragment;
	CharacterFragment.Shape = CharacterVirtualShape;
	CharacterFragment.Params = Params;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(CharacterFragment));
}

FVector URecallCharacterVirtualCollisionTrait::GetExtents() const
{
	return FVector(
		CharacterVirtualShape.StandingCapsule.Radius,
		CharacterVirtualShape.StandingCapsule.Radius,
		CharacterVirtualShape.StandingCapsule.HalfHeightOfCylinder + CharacterVirtualShape.StandingCapsule.Radius);
}
