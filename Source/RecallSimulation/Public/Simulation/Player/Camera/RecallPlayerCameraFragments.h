// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityTypes.h"
#include "RecallPlayerCameraTypes.h"
#include "System/Actor/RecallActorTypes.h"

#include "RecallPlayerCameraFragments.generated.h"

USTRUCT()
struct RECALLSIMULATION_API FRecallPlayerCameraFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString TargetPlayerID;

	// Follow the player
	UPROPERTY(VisibleAnywhere)
	bool bFollowPlayer = true;
};

USTRUCT()
struct RECALLSIMULATION_API FRecallPlayerCameraSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta=(AllowedClasses="/Script/Engine.Actor"))
	FSoftClassPath CameraActorClass;

	UPROPERTY(VisibleAnywhere)
	FRecallPlayerCameraSettings Settings;
};
