// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsObjectFactory.h"

#include "Physics/RecallPhysicsObjects.h"
#include "System/Physics/RecallPhysicsSubsystem.h"

TSharedPtr<FRecallPhysicsBody> URecallPhysicsObjectFactory::BuildPhysicsObject(
	uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const
{
	unimplemented();
	return nullptr;
}

void URecallPhysicsObjectFactory::SetupPhysicsObject(const TSharedPtr<FRecallPhysicsBody>& Body) const
{
	if (!ensure(Body.IsValid())) return;

	Body->SetWorldContextObject(GetOuter());

#if WITH_JOLT_PHYSICS
	const URecallPhysicsSubsystem* PhysicsSystem = CastChecked<URecallPhysicsSubsystem>(GetOuter());
	Body->SetPhysicsSystem(PhysicsSystem->physics_system);
	Body->SetTempAllocator(PhysicsSystem->temp_allocator);
#endif // WITH_JOLT_PHYSICS
}
