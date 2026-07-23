// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"
#include "System/Restore/RecallRestoreTypes.h"
#include "System/Simulation/RecallSimulationControllerTypes.h"

#include "RecallGameSimulationComponent.generated.h"

class ARecallPlayerControllerBase;

/**
 * Component to control the flow of the local game simulation.
 */
UCLASS()
class RECALLONLINE_API URecallGameSimulationComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	void SetGameSimStartParams(const FRecallSimulationStartParams& Params);
	const FRecallSimulationStartParams& GetGameSimStartParams() const { return SimStartParams; }

	void ResetPlayerEvents();
	const FRecallRestorePlayerArray& GetPlayerEvents() const { return PlayerEvents; }
	const FRecallRestoreWorldArray& GetWorldStates() const { return WorldStates; }

	uint32 GetServerSimulationFrame() const { return ReplicatedSimulationFrame; }
	void SetServerSimulationFrame(uint32 Frame);

	uint32 GetLocalSimulationFrame() const { return LocalSimulationFrame; }

	void OnRestoreSimulation();

public:
	void LaunchSimulation();
	void ResumeSimulation();
	void PauseSimulation();
	void ResetSimulation(bool bRestart = true, bool bAddPlayers = true);
	void PushSimulationInput(const FString& PlayerId, const TArray<FRecallFrameInput>& FrameInputs);

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface

	//~ Begin UObject Interface.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface.
	
#pragma region RPC
public:	
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_AddPlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_RemovePlayer(int32 WorldIndex, uint32 Frame, const FString& PlayerId);
#pragma endregion RPC

protected:
	/** Can the client restore its simulation state? */
	virtual bool CanResumeSimulation() const;
	
protected:
	/**
	* Simulation frame on the Host.
	* Does not take ping into account.
	*/
	UPROPERTY(Replicated)
	uint32 ReplicatedSimulationFrame{ 0 };

	/** Simulation frame for this client */
	UPROPERTY(Transient)
	uint32 LocalSimulationFrame { 0 };
	
	/* Start params for the game simulation. */
	UPROPERTY(Replicated)
	FRecallSimulationStartParams SimStartParams;
	
	/* Info to sync for each field */
	UPROPERTY(Replicated)
	FRecallRestoreWorldArray WorldStates;

	/**
	 * Replicated player join/leave events so it can be synced on every client.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_PlayerEvents)
	FRecallRestorePlayerArray PlayerEvents;

	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;
	
	UPROPERTY(Transient)
	bool bForceRestore{ false };
		
	UFUNCTION()
	void OnRep_PlayerEvents();

	void DeferredLaunchSimulation();
	void DeferredResumeSimulation();

	void LaunchSimulation_Internal();
	void ResumeSimulation_Internal();

	void DeferredRestoreSimulation();
	
	void OnFrameStart(uint32 Frame);
	void OnLoadSnapshot(uint32 Frame);

	void SetLocalSimulationFrame(uint32 Frame);
};
