// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "System/Actor/RecallActorTypes.h"
#include "Representation/Actor/RecallActorMeshRepresentationTypes.h"

#include "RecallActorRepresentationFragments.generated.h"

/**
 * Tag to identify a skeletal mesh actor representation.
 */
USTRUCT() struct FRecallSkeletalMeshActorRepresentationTag : public FMassTag { GENERATED_BODY() };

USTRUCT() struct FRecallOverrideActorAnimationRepresentationTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FRecallActorRepresentationFragment : public FMassFragment
{
	GENERATED_BODY()

	/**
	 * Handle of the actor representation.
	 */
	UPROPERTY(VisibleAnywhere)
	FRecallActorHandle ActorHandle;

	/**
	 * Handles of the attachments to the actor representation.
	 */
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallActorHandle> AttachmentActorHandles;

	/**
	 * Scale of the actor representation.
	 */
	UPROPERTY(VisibleAnywhere)
	FVector Scale = FVector::OneVector;

	/**
	 * Offset the actor representation relative to the entity's transform.
	 */
	UPROPERTY(VisibleAnywhere)
	FTransform Offset = FTransform::Identity;

	/**
	 * Toggle hidding the actor representation.
	 */
	UPROPERTY(VisibleAnywhere)
	bool bHideActor = false;
};

template <>
struct TMassFragmentTraits<FRecallActorRepresentationFragment> final
{ enum { AuthorAcceptsItsNotTriviallyCopyable = true }; };

USTRUCT()
struct FRecallSkeletalMeshActorRepresentationConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FSkeletalMeshRepresentationMeshDesc Definition;
};

USTRUCT()
struct FRecallStaticMeshActorRepresentationConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FStaticMeshRepresentationMeshDesc Definition;
};

USTRUCT()
struct FRecallActorRepresentationConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FActorRepresentationDesc Definition;
};

USTRUCT()
struct FRecallDecalActorRepresentationConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FRecallDecalActorRepresentationDesc Desc;
};
