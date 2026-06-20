// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Game/InGame/EasyOnlineGameState_InGame.h"

#include "RecallGameState_InGame.generated.h"

class UEasyOnlineBotCreationComponent;
class URecallClientRestoreComponent;
class URecallGameSimulationComponent;
class URecallJoinGameComponent;
class URecallLatencyGameComponent;
class ARecallPlayerState_InGame;
class URecallReplayGameComponent;
class URecallSyncInputGameComponent;
class URecallSyncReportGameComponent;

struct FInstancedStruct;

/**
 *
 */
UCLASS()
class RECALLONLINE_API ARecallGameState_InGame : public AEasyOnlineGameState_InGame
{
	GENERATED_UCLASS_BODY()

public:
	void SetGameEndReason(const FString& Reason);
	const FString& GetGameEndReason() const { return GameEndReason; }

	URecallGameSimulationComponent* GetGameSimulationComponentChecked() const { check(GameSimulationComponent); return GameSimulationComponent; }	
	URecallLatencyGameComponent* GetLatencyComponentChecked() const { check(LatencyComponent); return LatencyComponent; }	
	URecallSyncInputGameComponent* GetSyncInputComponentChecked() const { check(SyncInputComponent); return SyncInputComponent; }	
	URecallSyncReportGameComponent* GetSyncReportComponentChecked() const { check(SyncReportComponent); return SyncReportComponent; }	
	URecallClientRestoreComponent* GetClientRestoreComponentChecked() const { check(ClientRestoreComponent); return ClientRestoreComponent; }	
	URecallJoinGameComponent* GetJoinGameComponentChecked() const { check(JoinGameComponent); return JoinGameComponent; }	
	URecallReplayGameComponent* GetReplayComponentChecked() const { check(ReplayGameComponent); return ReplayGameComponent; }
	UEasyOnlineBotCreationComponent* GetBotCreationComponentChecked() const { check(BotCreationComponent); return BotCreationComponent; }
	
	/**
	 * Called when a local player join the game.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLocalJoinGame, ARecallPlayerState_InGame* /*PlayerState*/);
	FOnLocalJoinGame OnLocalJoinGame;

	/**
	 * Called when a local player leave the game.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLocalLeaveGame, ARecallPlayerState_InGame* /*PlayerState*/);
	FOnLocalLeaveGame OnLocalLeaveGame;

	//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor Interface

	//~ Begin AGameState Interface
protected:
	virtual void HandleMatchHasEnded() override;
	//~ End AGameState Interface
	
public:
	/**
	 * Set custom data from replay when saved.
	 */
	virtual void SaveReplayCustomData(FInstancedStruct& OutData) const {}
	
	/**
	 * Get custom data from replay when loaded.
	 */
	virtual void LoadReplayCustomData(const FInstancedStruct& Data) {}
	
	virtual bool CanLoadReplay() const;
	
protected:
	/**
	 * Whether the game simulation should be paused when the match ends.
	 */
	UPROPERTY(EditDefaultsOnly, Category=Game)
	bool bPauseGameSimulationOnEndMatch = true;
	
protected:
	/**
	 * Reason why the game simulation ended.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_GameEndReason)
	FString GameEndReason;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UEasyOnlineBotCreationComponent> BotCreationComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallGameSimulationComponent> GameSimulationComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallLatencyGameComponent> LatencyComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallSyncInputGameComponent> SyncInputComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallSyncReportGameComponent> SyncReportComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallClientRestoreComponent> ClientRestoreComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallJoinGameComponent> JoinGameComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallReplayGameComponent> ReplayGameComponent;
	
private:
	UFUNCTION()
	void OnRep_GameEndReason();

};
