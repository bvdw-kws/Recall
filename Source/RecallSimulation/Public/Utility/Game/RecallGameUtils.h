// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

class URecallRepresentationEventSubsystem;

/**
 * Utility methods to trigger game events or access game rule system's variables.
 */
namespace Recall::Game::Utils
{

/**
 * Event when the game ends.
 */
RECALLSIMULATION_API extern void EndGame(URecallRepresentationEventSubsystem& RepresentationEventSystem, const FString& Reason);
	
/**
 * Event when a controller is defeated.
 */
RECALLSIMULATION_API extern void ControllerDefeat(URecallRepresentationEventSubsystem& RepresentationEventSystem,
	const FString& ControllerID, const FString& Reason);

/**
 * Return the list of all the existing game rule assets.
 */
RECALLSIMULATION_API extern TArray<FName> GetGameRuleNames();

/**
 * Game rule's internal match time in seconds.
 */
RECALLSIMULATION_API extern double GetMatchTimeSeconds(const UObject* WorldContextObject);

} // namespace Recall::Game::Utils
