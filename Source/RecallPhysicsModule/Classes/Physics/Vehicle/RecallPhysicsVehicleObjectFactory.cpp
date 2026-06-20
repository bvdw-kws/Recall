// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsVehicleObjectFactory.h"

#include "Data/Physics/RecallPhysicsLayerDataAsset.h"
#include "RecallPhysicsVehicleObject.h"
#include "RecallPhysicsVehicleShapeTypes.h"

TSharedPtr<FRecallPhysicsBody> URecallPhysicsVehicleObjectFactory::BuildPhysicsObject(uint32 BodyID,
	const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsVehicleShape& VehicleShape = Shape.Get<FRecallPhysicsVehicleShape>();
	const TSharedPtr<FRecallPhysicsVehicleBody> VehicleBody = MakeShared<FRecallPhysicsVehicleBody>();
	
	SetupPhysicsObject(VehicleBody);
	
	const int32 Layer = URecallPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	VehicleBody->InitVehicle(VehicleShape, Params, BodyID, Layer);
	
	return VehicleBody;
}
