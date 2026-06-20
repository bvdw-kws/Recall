// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityTraitBase.h"
#include "Representation/Actor/RecallActorMeshRepresentationTypes.h"

#include "RecallActorRepresentationTraits.generated.h"

UCLASS(Abstract)
class RECALLSIMULATION_API URecallActorRepresentationTraitBase : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	/**
	 * Offset of the actor representation relative to the entity.
	 */
	UPROPERTY(EditAnywhere)
	FTransform Offset = FTransform::Identity;

	/**
	 * Base scale of the actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName="Scale")
	FVector Scale = FVector::OneVector;

	/**
	 * Apply the scale of the actor instance (in the level) to the entity.
	 */
	UPROPERTY(EditAnywhere)
	bool bUseActorScale = true;
};

UCLASS(meta=(DisplayName="MS Skeletal Mesh"))
class RECALLSIMULATION_API URecallSkeletalMeshActorRepresentationTrait : public URecallActorRepresentationTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	//~ Begin UObject Interface.
protected:
	virtual void PostLoad() override;
	//~ End UObject Interface.
	
protected:
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FSkeletalMeshRepresentationMeshDesc Mesh;
};

UCLASS(meta=(DisplayName="MS Static Mesh"))
class RECALLSIMULATION_API URecallStaticMeshActorRepresentationTrait : public URecallActorRepresentationTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	//~ Begin UObject Interface.
protected:
	virtual void PostLoad() override;
	//~ End UObject Interface.

protected:
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FStaticMeshRepresentationMeshDesc Mesh;
};

UCLASS(meta=(DisplayName="MS Actor"))
class RECALLSIMULATION_API URecallActorRepresentationTrait : public URecallActorRepresentationTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	//~ Begin UObject Interface.
protected:
	virtual void PostLoad() override;
	//~ End UObject Interface.

protected:
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FActorRepresentationDesc Actor;

	UPROPERTY()
	bool bIsSkeletalMeshActor = false;
};

UCLASS(meta=(DisplayName="MS Decal"))
class RECALLSIMULATION_API URecallDecalRepresentationTrait : public URecallActorRepresentationTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	//~ Begin UObject Interface.
protected:
	virtual void PostLoad() override;
	//~ End UObject Interface.

protected:
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FRecallDecalActorRepresentationDesc DecalActor;
};
