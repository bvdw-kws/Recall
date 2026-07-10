// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/ActorComponent.h"

#include "RecallGameEditorGameModeComponent.generated.h"

/**
 * Blocks the match from starting, and opens the Game Editor, while the
 * current map is configured to start in the Game Editor (see
 * ARecallWorldSettings::bStartInGameEditor).
 */
UCLASS(Within=RecallGameMode)
class RECALLONLINE_API URecallGameEditorGameModeComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Return false to block the match from starting.
	 */
	bool CanStartMatch() const;

	/**
	 * Whether the current map wants the Game Editor to be open.
	 */
	bool ShouldOpenGameEditor() const;

	/**
	 * Open the Game Editor and notify the local player's camera manager so it
	 * can clear the black screen fade it sets while waiting for the match to start.
	 * Does nothing if already in editor mode.
	 */
	void EnterGameEditorMode();

	/**
	 * Whether the Game Editor is currently open, as entered via EnterGameEditorMode().
	 */
	bool IsInGameEditorMode() const { return bIsInGameEditorMode; }

	/**
	 * Called from ARecallGameMode::StartPlay(), once actors like the player's
	 * camera manager have been spawned and are ready to be used.
	 */
	void OnStartPlay();

private:
	/**
	 * Whether we already entered the Game Editor, to avoid re-entering it twice.
	 */
	bool bIsInGameEditorMode = false;
};
