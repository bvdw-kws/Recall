// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/Collision/JPRPhysicsGroundCollisionTesterRay.h"

/// Class that does collision detection with ground
class RECALLPHYSICSMODULE_API FRecallPhysicsGroundCollisionTesterRay : public FJPRPhysicsGroundCollisionTesterRay
{
public:
	using FJPRPhysicsGroundCollisionTesterRay::FJPRPhysicsGroundCollisionTesterRay;
};
