// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

namespace Recall::Physics::ProcessorGroupNames
{
	
	// StartPhysics
	const FName Initialize = FName(TEXT("RecallPhysicsInitialize"));
	const FName SensorAttachment = FName(TEXT("RecallSensorAttachment"));

	// DuringPhysics
	const FName Update = FName(TEXT("RecallPhysicsUpdate"));
	const FName CharacterVirtualUpdate = FName(TEXT("RecallPhysicsCharacterVirtualUpdate"));
	const FName CharacterPostUpdate = FName(TEXT("RecallPhysicsCharacterPostUpdate"));

	// EndPhysics
	const FName CopyLocation = FName(TEXT("RecallPhysicsCopyLocation"));
	const FName GeneratesHitEvent = FName(TEXT("RecallPhysicsGeneratesHitEvent"));

	// PostPhysics
	const FName Overlap = FName(TEXT("RecallPhysicsOverlap"));

	// FrameEnd
	const FName ResetHitEvents = FName(TEXT("RecallPhysicsResetHitEvents"));

} // namespace Recall::Physics::ProcessorGroupNames
