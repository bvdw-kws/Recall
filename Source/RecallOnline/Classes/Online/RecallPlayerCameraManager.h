// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Camera/PlayerCameraManager.h"

#include "RecallPlayerCameraManager.generated.h"

UCLASS()
class RECALLONLINE_API ARecallPlayerCameraManager :
	public APlayerCameraManager
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Callback when the simulation has been started.
	 */
	virtual void OnStartSimulation();
	
	/**
	 * Callback when the simulation has begun.
	 * This means that the simulation has been ticked at least once so players can be spawned.
	 */
	virtual void OnBeginSimulation();

	/**
	 * Callback when the player possessed its view target.
	 * The game simulation pawn might not have been received yet.
	 */
	virtual void OnPossessViewTarget();
	
	/**
	 * Callback when the game simulation pawn has been received.
	 */
	virtual void OnReceiveSimulationPawn();

	/**
	 * Callback when the Game Editor is opened for this player, so the manual
	 * camera fade set in BeginPlay can be cleared and the screen isn't left black.
	 */
	virtual void OnEnterGameEditor();

	//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

protected:
	UPROPERTY(EditDefaultsOnly, meta=(Units=Seconds))
	float FadeOutDuration = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, meta=(Units=Seconds))
	float FadeInDuration = 0.5f;
	
};
