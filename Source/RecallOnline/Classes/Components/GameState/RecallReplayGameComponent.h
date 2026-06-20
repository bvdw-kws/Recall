// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallReplayGameComponent.generated.h"

/**
 * Component to synchronize a replay between players.
 * Wrapper for the replay subsystem with replication features.
 */
UCLASS()
class RECALLONLINE_API URecallReplayGameComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(Blueprintable, Category = "Replay")
	void StartReplay();

	UFUNCTION(BlueprintPure, Category = "Replay")
	bool IsPlayingReplay() const;
	
	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface

	//~ Begin UObject Interface.
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface.
	
#pragma region RPC
protected:
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_StartReplay();
#pragma endregion RPC

private:
	UPROPERTY(Replicated)
	bool bPlayingReplay = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallRestoreSubsystem> RestoreSystem;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallReplaySubsystem> ReplaySystem;

	void ServerRestoreAllRemoteClients() const;
	void ClientStartRestoringGameSimulation() const;
};
