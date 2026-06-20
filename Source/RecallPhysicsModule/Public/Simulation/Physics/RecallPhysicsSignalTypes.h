// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

namespace Recall::Physics::Signals
{

	/**
	 * Sensor begins to overlap with other collider(s).
	 */
	const FName OverlapBegin	= FName(TEXT("RecallPhysicsOverlapBegin"));
	
	/**
	 * Sensor finishes to overlap with other collider(s).
	 */
	const FName OverlapEnd		= FName(TEXT("RecallPhysicsOverlapEnd"));
	
	/**
	 * Sensor is overlapping with at least one other collider.
	 */
	const FName OverlapUpdate	= FName(TEXT("RecallPhysicsOverlapUpdate"));

	/**
	 * Sensor detected an overlap with a new entity.
	 */
	const FName NewOverlap		= FName(TEXT("RecallPhysicsNewOverlap"));
	
	/**
	 * Sensor lost an overlap with a new entity.
	 */
	const FName MinusOverlap		= FName(TEXT("RecallPhysicsMinusOverlap"));
	
	/**
	 * Collider detected a hit with another collider.
	 */
	const FName Hit				= FName(TEXT("RecallPhysicsHit"));
	
} // namespace Recall::Physics::Signals
