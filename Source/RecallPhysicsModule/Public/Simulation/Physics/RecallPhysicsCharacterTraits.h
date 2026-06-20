// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "RecallPhysicsBodyTrait.h"
#include "Physics/Character/RecallPhysicsCharacterShapeTypes.h"

#include "RecallPhysicsCharacterTraits.generated.h"

UCLASS(meta=(DisplayName="MS Character Collider"))
class RECALLPHYSICSMODULE_API URecallCharacterCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
	FRecallPhysicsCharacter CharacterShape;

protected:
	virtual FVector GetExtents() const override final;

};

UCLASS(meta=(DisplayName="MS Character Virtual Collider"))
class RECALLPHYSICSMODULE_API URecallCharacterVirtualCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRecallPhysicsCharacterVirtual CharacterVirtualShape;
	
protected:
	virtual FVector GetExtents() const override final;

};
