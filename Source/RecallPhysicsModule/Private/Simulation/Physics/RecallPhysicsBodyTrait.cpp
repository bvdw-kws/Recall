// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Physics/RecallPhysicsBodyTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsSensorFragment.h"
#include "Simulation/Transform/RecallTransformFragments.h"

//----------------------------------------------------------------------//
// URecallPhysicsBodyTrait
//----------------------------------------------------------------------//
void URecallPhysicsBodyTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildBodyTemplate(BuildContext, World, GetStartParameters(),
		TransformCopy, GetExtents(), Params.IsStatic());
}

ERecallPhysicsBodyStartParameters URecallPhysicsBodyTrait::GetStartParameters() const
{
	ERecallPhysicsBodyStartParameters StartParameters = ERecallPhysicsBodyStartParameters::None;

	if (bSimulationGeneratesHitEvent)
	{
		StartParameters |= ERecallPhysicsBodyStartParameters::GeneratesHitEvent;
	}

	if (bStartsEnabled)
	{
		StartParameters |= ERecallPhysicsBodyStartParameters::StartsEnabled;
	}

	return StartParameters;
}

void URecallPhysicsBodyTrait::BuildBodyTemplate(FMassEntityTemplateBuildContext& BuildContext,
	const UWorld& World, ERecallPhysicsBodyStartParameters StartParams,
	EJPRPhysicsTransformCopyParameters TransformCopyParams,
	const FVector& Extents, bool bIsStatic)
{
	BuildContext.RequireFragment<FRecallTransformFragment>();

	FRecallPhysicsBodyFragment& BodyFragment = BuildContext.AddFragment_GetRef<FRecallPhysicsBodyFragment>();
	BodyFragment.Extents = Extents;
	BodyFragment.StartParameters = static_cast<uint8>(StartParams);
	BodyFragment.TransformCopy = static_cast<uint8>(TransformCopyParams);

	BuildContext.AddFragment<FRecallPhysicsSensorFragment>();
	
	if (bIsStatic)
	{
		BuildContext.AddTag<FRecallPhysicsStaticColliderTag>();
	}
}
