// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallJoinGameComponent.generated.h"

class ARecallPlayerControllerBase;

/**
 * Component to handle controllers joining and leaving the game simulation.
 */
UCLASS()
class RECALLONLINE_API URecallJoinGameComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Make all the controllers join the game, useful at the start of the match.
	 */
	void JoinGameAll();
	
	/**
	 * Make this controller join the game simulation.
	 */
	void JoinGame(AController& Controller, bool bDebugJoin = false);
	
	/**
	 * Make this controller leave the game simulation.
	 */
	void LeaveGame(AController& Controller);

	/**
	 * Make this player spectate the game.
	 */
	void SpectateGame(ARecallPlayerControllerBase& PlayerController, bool bLaunchGame = false);

	/**
	 * Respawn the entity for all the controllers.
	 */
	void RespawnPlayerEntities() const;

	/**
	 * Handle a client who just finished restoring the simulation.
	 */
	void FinishRestoring(ARecallPlayerControllerBase& PlayerController);

	/**
	 * Respawn local player in the game simulation with a new character.
	 */
	void SetLocalPlayerCharacter(const FName& CharacterName, int32 LocalPlayerIndex = 0) const;

	/**
	 * Return how many controllers have currently joined the game simulation.
	 */
	int32 GetNumJoinGameSimPlayers() const { return NumJoinGameSimPlayers; }
	
protected:
	/**
	 * Keep track of how many controllers joined the game simulation.
	 * A controller can join the session but not the game simulation.
	 */
	UPROPERTY(Transient)
	int32 NumJoinGameSimPlayers = 0;
	
	/**
	 * Return the frame where it should be safe to add a new controller's entity.
	 */
	uint32 GetPlayerChangeSafeFrame() const;

};
