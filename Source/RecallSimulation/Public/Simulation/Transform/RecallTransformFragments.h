// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "Mass/EntityHandle.h"

#include "RecallTransformFragments.generated.h"

USTRUCT()
struct RECALLSIMULATION_API FRecallTransformFragment : public FMassFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere)
	FQuat Rotation = FQuat::Identity;
	
	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle AttachParent;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FMassEntityHandle> AttachChildren;

	FORCEINLINE FTransform GetTransform() const { return FTransform(Rotation, Position); }

	FORCEINLINE bool HasChildren() const { return AttachChildren.Num() > 0; }
	FORCEINLINE bool HasParent() const { return AttachParent.IsSet(); }
};

template <>
struct TMassFragmentTraits<FRecallTransformFragment> final
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};
