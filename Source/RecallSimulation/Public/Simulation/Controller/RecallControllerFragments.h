// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityElementTypes.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "Mass/EntityHandle.h"
#include "UObject/ObjectPtr.h"

#include "RecallControllerFragments.generated.h"

class UMassEntityConfigAsset;

// Tag to identify player controlled entity
USTRUCT() struct FRecallPlayerControllerTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct RECALLSIMULATION_API FRecallControllerFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString ControllerID;

	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle CameraEntity;
	
	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle ViewTargetEntity;
	
	UPROPERTY(VisibleAnywhere)
	FRotator ControlRotation = FRotator::ZeroRotator;

public:
	int32 GetRepresentationPlayerIndex(const UObject* WorldContextObject) const;
	static int32 GetRepresentationPlayerIndex(const UObject* WorldContextObject, const FString& PlayerID);
};

template <>
struct TMassFragmentTraits<FRecallControllerFragment> final
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};

USTRUCT()
struct RECALLSIMULATION_API FRecallPlayerControllerSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<const UMassEntityConfigAsset> CameraEntityConfig;
};
