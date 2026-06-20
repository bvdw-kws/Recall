// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Controller/RecallControllerFragments.h"

#include "Utility/Player/RecallPlayerUtils.h"

int32 FRecallControllerFragment::GetRepresentationPlayerIndex(const UObject* WorldContextObject) const
{
	return GetRepresentationPlayerIndex(WorldContextObject, ControllerID);
}

int32 FRecallControllerFragment::GetRepresentationPlayerIndex(const UObject* WorldContextObject, const FString& PlayerID)
{
	int32 PlayerIndex = INDEX_NONE;
	Recall::Player::Utils::FindLocalPlayerIndex(WorldContextObject, PlayerID, PlayerIndex);
	return PlayerIndex;
}
