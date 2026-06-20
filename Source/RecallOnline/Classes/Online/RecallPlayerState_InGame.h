// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Game/InGame/EasyOnlinePlayerState_InGame.h"
#include "Player/Interface/RecallPlayerStateInterface.h"

#include "RecallPlayerState_InGame.generated.h"

class URecallInputLatencyPlayerComponent;
class URecallSyncReportPlayerComponent;
struct FRecallRestoreInputBunch;

UCLASS()
class RECALLONLINE_API ARecallPlayerState_InGame :
	public AEasyOnlinePlayerState_InGame,
	public IRecallPlayerStateInterface
{
	GENERATED_UCLASS_BODY()

public:
	void SetSimPlayerId(const FString& InSimPlayerId);
	void ClearSimPlayerId();

	/** Confirm frame for this client. */
	void SetLocalConfirmFrame(uint32 Frame);
	uint32 GetLocalConfirmFrame() const;

	/** Last received input frame for this client*/
	void SetLastReceivedInputFrame(uint32 Frame);
	uint32 GetLastReceivedInputFrame() const;

	// Join
	void SetJoinedGame(bool bInJoinedGame, bool bInDebugJoin = false);
	bool HasJoinedGame() const { return bJoinedGame; }
	bool IsDebugJoin() const { return bDebugJoin; }
	
	// Restore
	void SetIsRestoring(bool bInRestoring);
	bool IsRestoring() const { return bIsRestoring; }
	
	void SetRestoreTargetFrame(uint32 Frame);
	uint32 GetRestoreTargetFrame() const;

	// Leaving
	void SetIsLeavingGame();
	void ClearIsLeavingGame();
	bool IsLeavingGame() const;

	// Defeat
	void SetDefeatReason(const FString& Reason);
	const FString& GetDefeatReason() const { return DefeatReason; }
	
	URecallSyncReportPlayerComponent* GetSyncReportComponent() const { return SyncReportComponent; }
	URecallSyncReportPlayerComponent* GetSyncReportComponentChecked() const { check(SyncReportComponent); return SyncReportComponent; }

	URecallInputLatencyPlayerComponent* GetInputLatencyComponent() const { return InputLatencyComponent; }
	URecallInputLatencyPlayerComponent* GetInputLatencyComponentChecked() const { check(InputLatencyComponent); return InputLatencyComponent; }
	
	//~ Begin IRecallPlayerStateInterface Interface
public:
	const FString& GetSimPlayerId() const override { return SimPlayerId; }
	//~ End IRecallPlayerStateInterface Interface

	//~ Begin AActor Interface
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void PostInitializeComponents() override;
	//~ End AActor Interface

	//~ Begin APlayerState Interface
protected:
	virtual void OnDeactivated() override;
	//~ End APlayerState Interface

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallSyncReportPlayerComponent> SyncReportComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallInputLatencyPlayerComponent> InputLatencyComponent;
	
	UPROPERTY(Replicated)
	FString SimPlayerId;

	UPROPERTY(ReplicatedUsing=OnRep_JoinedGame)
	bool bJoinedGame = false;
	
	UPROPERTY(Transient)
	bool bDebugJoin = false;

	/**
	 * Whether this client is in the process of restoring the game simulation state.
	 */
	UPROPERTY(Replicated)
	bool bIsRestoring = false;

	/**
	 * The simulation should finishes restore once the target frame has been reached for this client.
	 */
	UPROPERTY(Replicated)
	uint32 RestoreTargetFrame = 0;

	/**
	 * Frame at which this player left the game simulation.
	 */
	UPROPERTY(Replicated)
	bool bIsLeavingGame = false;

	/**
	 * Reason why this player lost.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_DefeatReason)
	FString DefeatReason;
	
#pragma region RPC
public:
	UFUNCTION(Client, Reliable)
	void Client_RestoreSimulation();
#pragma endregion RPC

private:
	/* Confirm frame for this player, used to evaluate the shared confirm frame. */
	UPROPERTY(Transient)
	uint32 LocalConfirmFrame{ 0 };

	UPROPERTY(Transient)
	uint32 LastReceivedInputFrame { 0 };

	UPROPERTY(Transient)
	bool bWaitLeavingOnDeactivated = false;

private:
	UFUNCTION()
	void OnRep_JoinedGame();

	UFUNCTION()
	void OnRep_DefeatReason();
};
