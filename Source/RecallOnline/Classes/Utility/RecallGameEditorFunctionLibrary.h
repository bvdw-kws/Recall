// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "RecallGameEditorFunctionLibrary.generated.h"

UCLASS()
class RECALLONLINE_API URecallGameEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Marks the match as ready to start from the Game Editor, letting
	// ARecallGameMode start the match even while the current map wants the
	// Game Editor to be open.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void PlayGame(const UObject* WorldContextObject);

	// Whether the match can be exited back to the Game Editor, i.e. whether
	// the current map wants the Game Editor to be open.
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool CanExitPlayMode(const UObject* WorldContextObject);

	// Exits the match back to the Game Editor, resetting the game mode to
	// WaitingToStart so the game simulation can be reset.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void ExitPlayMode(const UObject* WorldContextObject);
};
