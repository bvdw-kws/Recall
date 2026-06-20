// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSimulationSettings.h"

#include "Data/Physics/RecallPhysicsLayerDataAsset.h"
#include "UObject/ConstructorHelpers.h"

URecallSimulationSettings::URecallSimulationSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{	
	static ConstructorHelpers::FObjectFinder<URecallPhysicsLayerDataAsset> DefaultLayerAsset(TEXT("/Recall/DataAssets/Physics/DA_PhysicsLayer_Default"));
	DefaultLayer = DefaultLayerAsset.Object;
}
