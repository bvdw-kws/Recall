// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallGameEditorGameComponent.generated.h"

/**
 * Blocks the match from starting, and opens the Game Editor, while the
 * current map is configured to start in the Game Editor (see
 * ARecallWorldSettings::bStartInGameEditor).
 */
UCLASS()
class RECALLONLINE_API URecallGameEditorGameComponent : public UGameStateComponent
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
	 * Does nothing if already in editor mode. Only the server can call this,
	 * the state is then replicated to clients via bIsInGameEditorMode.
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

	//~ Begin UObject Interface.
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface.

protected:
	/**
	 * Whether we already entered the Game Editor, to avoid re-entering it twice.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_IsInGameEditorMode, Transient)
	bool bIsInGameEditorMode = false;

	/**
	 * Notify the local player's camera manager that the Game Editor is open, so it
	 * can clear the black screen fade it sets while waiting for the match to start.
	 */
	void OnEnterGameEditor();

private:
	UFUNCTION()
	void OnRep_IsInGameEditorMode();

protected:
#ifdef WITH_GAME_EDITOR_RUNTIME
	/**
	 * The Game Editor map layout configured for the current map, if any.
	 */
	class UGameEditorMapAsset* GetGameEditorMapAsset() const;
#endif // WITH_GAME_EDITOR_RUNTIME

};
