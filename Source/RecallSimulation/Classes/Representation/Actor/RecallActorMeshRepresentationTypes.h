// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "RecallActorRepresentationTypes.h"

#include "RecallActorMeshRepresentationTypes.generated.h"

USTRUCT(BlueprintType)
struct RECALLSIMULATION_API FLevelSequenceActorRepresentationDesc : public FRecallActorRepresentationDesc
{
	GENERATED_BODY()

	FLevelSequenceActorRepresentationDesc();

	/** Blueprint used for the representation of our entity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/LevelSequence.LevelSequenceActor"))
	FSoftClassPath Blueprint;

	/** Level sequence used by the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/LevelSequence.LevelSequence"))
	FSoftObjectPath LevelSequenceAsset;

	virtual FSoftClassPath GetSoftClassPath() const override;
};

/**
 * Base struct for an actor representation
 */
USTRUCT(BlueprintType)
struct FRecallRepresentationDescBase : public FRecallActorRepresentationDesc
{
	GENERATED_BODY()

	FRecallRepresentationDescBase() = default;
	FRecallRepresentationDescBase(const TSubclassOf<URecallActorRepresentationPoolFactory>& InFactoryClass)
		: Super(InFactoryClass) {}
	
	/**
	 * Base scale of the actor.
	 */
	UPROPERTY(meta=(DeprecatedProperty))
	FVector MeshScale_DEPRECATED = FVector::OneVector;
};

USTRUCT(BlueprintType)
struct RECALLSIMULATION_API FActorRepresentationDesc : public FRecallRepresentationDescBase
{
	GENERATED_BODY()

	FActorRepresentationDesc();

	/**
	 * Blueprint used for the representation of our entity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.Actor"))
	FSoftClassPath Blueprint;

	/**
	 * Reset all materials on init, such as UMeshComponent and UDecalComponent materials.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bResetMaterialsOnInit = true;

	virtual FSoftClassPath GetSoftClassPath() const override;
};

USTRUCT(BlueprintType)
struct RECALLSIMULATION_API FMeshRepresentationMaterialDesc
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName MaterialSlotName{ NAME_None };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
	FSoftObjectPath MaterialInterface;
};

USTRUCT(BlueprintType)
struct RECALLSIMULATION_API FSkeletalMeshRepresentationMeshDesc : public FRecallRepresentationDescBase
{
	GENERATED_BODY()

	FSkeletalMeshRepresentationMeshDesc();
	
	/**
	 * Blueprint used for the representation of our entity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.SkeletalMeshActor"))
	FSoftClassPath Blueprint;

	/** The skeletal mesh visual representation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.SkeletalMesh"))
	FSoftObjectPath Mesh;
	
	/** Material overrides for the skeletal mesh visual representation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMeshRepresentationMaterialDesc> MaterialOverrides;

	/** The animation blueprint to use on the visual representation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.AnimInstance"))
	FSoftClassPath AnimBP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
	FSoftObjectPath OverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CustomDepthStencilValue = 5;

	virtual FSoftClassPath GetSoftClassPath() const override;
};

USTRUCT(BlueprintType)
struct RECALLSIMULATION_API FStaticMeshRepresentationMeshDesc : public FRecallRepresentationDescBase
{
	GENERATED_BODY()

	FStaticMeshRepresentationMeshDesc();
	
	/**
	 * Blueprint used for the representation of our entity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.StaticMeshActor"))
	FSoftClassPath Blueprint;
	
	/** The static mesh visual representation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.StaticMesh"))
	FSoftObjectPath Mesh;

	/** Material overrides for the static mesh visual representation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMeshRepresentationMaterialDesc> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
	FSoftObjectPath OverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CustomDepthStencilValue = 5;

	virtual FSoftClassPath GetSoftClassPath() const override;
};

USTRUCT(BlueprintType)
struct RECALLSIMULATION_API FRecallDecalActorRepresentationDesc : public FRecallRepresentationDescBase
{
	GENERATED_BODY()

	FRecallDecalActorRepresentationDesc();

	/**
	 * Blueprint used for the representation of our entity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.DecalActor"))
	FSoftClassPath Blueprint;

	/**
	 * Material used by the decal.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
	FSoftObjectPath Material;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FColor DecalColor = FColor::White;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector DecalSize = static_cast<FVector>(100.0);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DecalSortOrder = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units=Seconds))
	float DecalFadeInStartDelay = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units=Seconds))
	float DecalFadeInDuration = 0.5f;

	virtual FSoftClassPath GetSoftClassPath() const override;
};
