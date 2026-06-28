// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityTraitBase.h"
#include "Physics/RecallPhysicsTypes.h"

#include "RecallPhysicsBodyTrait.generated.h"

enum class ERecallPhysicsBodyStartParameters : uint8;

UCLASS(Abstract)
class RECALLPHYSICSMODULE_API URecallPhysicsBodyTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

public:
	virtual FVector GetExtents() const { return FVector::ZeroVector; }
	virtual ERecallPhysicsBodyStartParameters GetStartParameters() const;

public:
	static void BuildBodyTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World,
		ERecallPhysicsBodyStartParameters StartParams, EJPRPhysicsTransformCopyParameters TransformCopyParams,
		const FVector& Extents, bool bIsStatic = false);
	
protected:
	/** Hit events will be generated for this entity so they can be accessed from the physics system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision)
	bool bSimulationGeneratesHitEvent = false;

	/** Enable the physics body for this entity as soon as it has been spawned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision)
	bool bStartsEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision)
	EJPRPhysicsTransformCopyParameters TransformCopy = EJPRPhysicsTransformCopyParameters::All;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision)
	FJPRPhysicsBodyParameters Params;
};
