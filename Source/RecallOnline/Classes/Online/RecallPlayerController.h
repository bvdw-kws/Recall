// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Base/RecallPlayerControllerBase.h"
#include "Player/Interface/RecallPlayerControllerInterface.h"
#include "Online/RecallInputNetSerializers.h"

#include "RecallPlayerController.generated.h"

struct FInputActionValue;

/**
 * Controller for players that can join the game and play directly.
 * This class should only contain logic to join the game or push input to its pawn.
 */
UCLASS()
class RECALLONLINE_API ARecallPlayerController :
	public ARecallPlayerControllerBase,
	public IRecallPlayerControllerInterface
{
	GENERATED_UCLASS_BODY()

public:
	FString GetPawnSimPlayerId() const;
	virtual bool IsLockInput() const;

public:
	UFUNCTION(BlueprintPure, Category="Recall|PlayerController")
	virtual bool CanPossessViewTarget() const;

	/**
	 * Callback when our pawn receives the game simulation pawn.
	 */
	virtual void ReceiveGameSimPawn();
	
#pragma region RPC
protected:
	/**
	 * Request the server to join the game.
	 */
	UFUNCTION(Server, Reliable)
	void Server_JoinGame();

	/**
	 * Request to possess the bot that this controller is currently spectating.
	 */
	UFUNCTION(Server, Reliable)
	void Server_PossessViewTarget();

	/**
	 * Callback when the host has approved possess view target.
	 */
	UFUNCTION(Client, Reliable)
	void Client_PossessViewTarget();
#pragma endregion RPC

	//~ Begin IRecallPlayerControllerInterface Interface
public:
	virtual void SetInputOptions(const FString& Options, bool bOverride = false) override;
	virtual const FString& GetInputOptions() const override;
	//~ End IRecallPlayerControllerInterface Interface

	//~ Begin APlayerController Interface
protected:
	virtual void BeginPlayingState() override;
	virtual void EndPlayingState() override;
	virtual void ReceivedPlayer() override;
	virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<class AActor*>& ActorList) override;
	virtual bool CanRestartPlayer() override;
	//~ End APlayerController Interface

	//~ Begin AActor Interface
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	//~ End APlayerController Interface

protected:
	/**
	 * Send the mouse position to the game simulation.
	 * Both the screen position and its world position (see GetGameSimulationMouseWorldPosition) are sent.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Playing")
	bool bUseGameSimulationMousePosition = false;

	/**
	 * Height of the horizontal plane the mouse is projected onto to get its world position.
	 * Used by the default GetGameSimulationMouseWorldPosition.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Playing", meta=(EditCondition="bUseGameSimulationMousePosition"))
	double GameSimulationMousePlaneZ = 0.0;

	/**
	 * Get the mouse position in world space, to send to the game simulation.
	 * Needed because screen positions are only meaningful for the local viewport.
	 * @return false if the mouse can't be projected, the previous world position is then kept.
	 */
	virtual bool GetGameSimulationMouseWorldPosition(FVector& OutWorldPosition) const;

	/**
	 * Project the mouse on a horizontal plane at the given height.
	 */
	bool DeprojectMouseToHorizontalPlane(double PlaneZ, FVector& OutWorldPosition) const;

	UPROPERTY(EditDefaultsOnly, Category="Input|Playing")
	TObjectPtr<UCurveFloat> LeftStickRemappingCurve;

	UPROPERTY(EditDefaultsOnly, Category="Input|Playing", meta=(RowType="/Script/RecallCore.RecallInputActionTableRow"))
	TObjectPtr<const UDataTable> InputActionTable;

	UPROPERTY(EditDefaultsOnly, Category="Input|Playing")
	TObjectPtr<UInputMappingContext> PlayingInputMapping;

	/**
	 * Action to set the left axis.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Playing")
	TObjectPtr<UInputAction> MovementAction;
	
	/**
	 * Action to set the right axis.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Input|Playing")
	TObjectPtr<UInputAction> TurnAtAction;
	
	UPROPERTY(EditDefaultsOnly, Category="Input|Spectating")
	TObjectPtr<UInputAction> JoinAction;
	
	UPROPERTY(EditDefaultsOnly, Category="Input|Spectating")
	TObjectPtr<UInputAction> PossessBotAction;

protected:
	void OnActionTriggered(const FInputActionValue& Value, int32 InputIndex);
	void OnActionCompleted(const FInputActionValue& Value, int32 InputIndex);
	void SetInputState(int32 InputIndex, bool Value);

	void OnMovementAction(const FInputActionValue& Value);
	void OnTurnAtAction(const FInputActionValue& Value);
	void OnJoinGameAction(const FInputActionValue& Value);
	void OnPossessBotAction(const FInputActionValue& Value);

protected:
	UPROPERTY(Transient)
	FRecallInput CurrentInput;

	void PushInput(const FRecallInput& Input);
	void ClearCurrentInput();

	void OnToggleDebugMenu(bool bOpen);

	void JoinGame();
	void PossessViewTarget();

};
