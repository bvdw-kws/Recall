// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Game/InGame/EasyOnlineGameMode_InGame.h"
#include "Observer/Game/RecallGameReactInterface.h"

#include "RecallGameMode.generated.h"

class ARecallGameState_InGame;
class ARecallPlayerController;
class ARecallPlayerControllerBase;
class ARecallPlayerState_InGame;
struct FInstancedStruct;
struct FRecallPlayerSpawnParameters;

/**
 * Game mode that manages our fixed step game simulation.
 */
UCLASS()
class RECALLONLINE_API ARecallGameMode :
	public AEasyOnlineGameMode_InGame,
	public IRecallGameReactInterface
{
	GENERATED_UCLASS_BODY()

public:
	void ResetConfirmFrame(uint32 Frame);

	/**
	 * List of all the character entities that can be spawned and controlled in the game simulation.
	 */
	TArray<FName> GetAllCharacterNames() const;

	/**
	 * Force the game to end.
	 */
	void EndGame(const FString& Reason);

	/**
	 * Exits the match back to WaitingToStart, e.g. to let the Game Editor be
	 * reopened. See ARecallGameMode::HandleMatchIsWaitingToStart().
	 */
	void ExitToWaitingToStart();

public:
	/**
	 * Called when a player finished traveling to map and is ready.
	 */
	virtual void SyncReady(AEasyOnlinePlayerController_InGame* PlayerController) override;

	/**
	 * Return true if this player is allowed to join the game.
	 */
	virtual bool CanJoinGame(ARecallPlayerControllerBase& PlayerController) const;

	/**
	 * Return true if a new player is allowed to join the game.
	 */
	virtual bool CanNewPlayerJoinGame() const;

	/**
	 * Generate an unique player ID for the game simulation.
	 */
	virtual void AssignPlayerId(AController& Controller);

	/**
	 * Release an existing player ID for the game simulation, so it can be reused.
	 */
	virtual void FreePlayerId(ARecallPlayerState_InGame& InGamePlayerState);
	
	/**
	 * Return true if we can save a snapshot of the game simulation.
	 */
	virtual bool CanSaveSnapshot() const;
	
	/**
	 * Return true if we can load a snapshot of the game simulation.
	 */
	virtual bool CanLoadSnapshot() const;

	/**
	 * Allow to pass custom spawn parameters for this player.
	 */
	virtual void SetCustomSpawnParams(const FString& PlayerSimID, FInstancedStruct& CustomParameters) {}

	/**
	 * Join the game instead of another player.
	 */
	virtual void SwapPlayers(AController& OldController, AController& NewController);

	/**
	 * Helper methods to get the game simulation character entity config path for a player.
	 */
	virtual FSoftObjectPath GetPlayerEntityConfig(const AController& Controller) const;
	
	/**
	 * Get game simulation spawn parameters for this player.
	 */
	virtual FRecallPlayerSpawnParameters GetPlayerSpawnParameters(const AController& Controller);

protected:
	/**
	 * Launch the game simulation for all the players that can join the game.
	 */
	virtual void LaunchGameSimulation();
	
	//~ Begin IRecallGameReactInterface Interface
protected:
	virtual void OnGameEnd(const FString& Reason) override;
	virtual void OnControllerDefeat(const FString& ControllerID, const FString& Reason) override;
	//~ End IRecallGameReactInterface Interface
	
	//~ Begin AActor Interface
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor Interface
	
	//~ Begin AGameModeBase Interface
public:
	virtual void InitGameState() override;
	virtual void InitSeamlessTravelPlayer(AController* NewController) override;
	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;
	virtual void StartPlay() override;
	virtual void GenericPlayerInitialization(AController* C) override;

protected:
	virtual void Logout(AController* Exiting) override;
	virtual int32 GetNumPlayers() override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	virtual bool CanSpectate_Implementation(APlayerController* Viewer, APlayerState* ViewTarget) override;
	virtual bool AllowCheats(APlayerController* P) override;
	//~ End AGameModeBase Interface

	//~ Begin AGameMode Interface
protected:
	virtual void RestartGame() override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	//~ End AGameMode Interface

protected:
	/**
	 * Default game simulation character for each player.
	 */
	UPROPERTY(EditAnywhere, Category=Player, meta=(GetOptions="GetCharacterNames", EditFixedOrder))
	FName DefaultPlayerCharacterName;

	/**
	 * Default game simulation character for each player.
	 */
	UPROPERTY(EditAnywhere, Category=Player, meta=(GetOptions="GetCharacterNames", EditFixedOrder))
	FName DefaultAICharacterName = NAME_None;
	
	/**
	 * Data table that contains all the playable game simulation characters.
	 */
	UPROPERTY(EditAnywhere, Category=Player, meta=(RowType="/Script/RecallCore.RecallCharacterDataTable"))
	TObjectPtr<const class UDataTable> PlayableCharacters;
	
	/**
	 * Name of the option to force the local player two to join at the start of the game.
	 */
	UPROPERTY(EditAnywhere, Category=Player)
	FString TwoPlayerOption{ TEXT("bTwoPlayer") };

	/**
	 * Game rule to default to if none is selected.
	 */
	UPROPERTY(EditAnywhere, Category=Game, meta=(GetOptions="GetGameRuleNames"))
	FName DefaultGameRule = NAME_None;
	
	/**
	 * Timer to limit how long the host waits for other players before disconnecting.
	 */
	UPROPERTY(EditAnywhere, Category=Game, meta=(Units="Seconds"))
	float WaitTravellingPlayersDuration = 30.0f;

	/**
	 * Delay before launching the game simulation once the game is ready.
	 */
	UPROPERTY(EditAnywhere, Category=Game, meta=(Units="Seconds"))
	float StartMatchDelay = 1.0f;

protected:
	/**
	 * Keep track of the local player controller for player two.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ARecallPlayerController> PlayerTwoLocalController;

	/**
	 * Restore system that is used to restore the game for remote players if they drop in during a game.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallRestoreSubsystem> RestoreSystem;

	/**
	 * Variable to toggle the local multiplayer splitscreen.
	 */
	UPROPERTY(Transient)
	bool bAllowSplitscreen = true;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallGameEditorGameComponent> GameEditorComponent;
	
	/**
	 * Incremented each time a new player join to assign him a unique ID.
	 */
	UPROPERTY(Transient)
	int32 GameSimPlayerIndexGenerator = 0;

	UPROPERTY(Transient)
	int32 GameSimBotIndexGenerator = 0;
	
	UPROPERTY(Transient)
	int32 GameSimSpectatorIndexGenerator = 0;

	/**
	 * Pool of free player ids so they can be reused.
	 */
	UPROPERTY(Transient)
	TArray<FString> FreePlayerIds;

	/**
	 * Timer handle to disconnect the host after waiting for other players for too long.
	 */
	UPROPERTY(Transient)
	FTimerHandle WaitTravellingPlayersTimerHandle;

	/**
	 * Helper methods to get the game simulation character name for a player.
	 */
	FName GetPlayerDefaultCharacterName(const AController& Controller) const;
	
	/**
	 * Helper methods to get the game simulation character entity config for a character name.
	 */
	FSoftObjectPath GetCharacterEntityConfig(const FName& CharacterName) const;

	/**
	 * How many players are expected, and how many players are ready to start the game.
	 */
	void GetNumPlayersSyncedAndReady(int& OutNumExpected, int& OutNumSynced) const;
	
	/**
	 * List of the names for all the playable characters.
	 */
	UFUNCTION()
	TArray<FName> GetCharacterNames();

	/**
	 * Listo f the names for all the existing game rules.
	 */
	UFUNCTION()
	TArray<FName> GetGameRuleNames();

	/**
	 * Callback after waiting for too long for other players to sync after traveling to the game.
	 */
	void OnWaitTravellingPlayersTimerComplete();

	/**
	 * Toggle the splitscreen if it is allowed and if we have more than one local players who joined the game simulation.
	 */
	void UpdateSplitscreen() const;

	ARecallGameState_InGame* GetInGameStateChecked() const;
};
