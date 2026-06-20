// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Game/InGame/EasyOnlinePlayerController_InGame.h"

#include "RecallPlayerControllerBase.generated.h"

class UInputMappingContext;
class UInputAction;

class URecallMultiSimControllerComponent;
class URecallInputControllerComponent;

/**
 * Base class shared by all the recall player controllers.
 */
UCLASS()
class RECALLONLINE_API ARecallPlayerControllerBase :
	public AEasyOnlinePlayerController_InGame
{
	GENERATED_UCLASS_BODY()

public:
	FString GetDefaultSimPlayerId() const;
	URecallMultiSimControllerComponent* GetMultiSimComponentChecked() const;
	URecallInputControllerComponent* GetGameSimulationInputComponentChecked() const;

	bool IsPlayerOne() const;
	bool HasJoinedGame() const;

	UFUNCTION(BlueprintCallable)
	void RequestSpectate();

public:
	/**
	 * Callback when the game simulation starts but might not have been ticked yet.
	 */
	virtual void OnStartSimulation();
	
	/**
	 * Callback after the game simulation has been stepped for the first time.
	 */
	virtual void OnBeginSimulation();

	/**
	 * Callback when this player lost in the game simulation.
	 */
	virtual void OnPlayerDefeat(const FString& Reason);
	
#pragma region RPC
public:
	UFUNCTION(Client, Reliable)
	void Client_LaunchGameSimulation();

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSpectate();
#pragma endregion RPC

#pragma region Spectator
protected:
	/**
	 * Input mapping while spectating.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Spectating")
	TObjectPtr<UInputMappingContext> SpectatorInputMapping;

	/**
	 * Action to spectate the next player.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Spectating")
	TObjectPtr<UInputAction> PreviousPlayerAction;
	
	/**
	 * Action to spectate the previous player.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Spectating")
	TObjectPtr<UInputAction> NextPlayerAction;

	void OnPreviousPlayer();
	void OnNextPlayer();
#pragma endregion Spectator

	//~ Begin AActor Interface
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;
	//~ End AActor Interface
	
	//~ Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	virtual void BeginSpectatingState() override;
	virtual void EndSpectatingState() override;
	//~ End APlayerController Interface

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallMultiSimControllerComponent> MultiSimComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallInputControllerComponent> GameSimulationInputComponent;

};
