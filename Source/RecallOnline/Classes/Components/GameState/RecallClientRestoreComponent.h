// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"
#include "DataTransfer/EasyDataTransferTypes.h"

#include "RecallClientRestoreComponent.generated.h"

struct FRecallSimulationStartParams;
struct FRecallRestoreInputBunch;
class UEasyDataTransferSubsystem;

/**
 * Component to restore the game simulation of a client.
 */
UCLASS()
class RECALLONLINE_API URecallClientRestoreComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	/* Initialize our game events */
	void InitGameSimulation(bool bSeed = true);

	/* Begin restoring asynchronously our game simulation */
	void RestoreGameSimulation(const FRecallSimulationStartParams& StartParams, bool bStart = true);
	void StartGameSimulation(const FRecallSimulationStartParams& StartParams);

	bool HasStarted() const;

	/* Stop restoring our game simulation */
	void StopRestoreGameSimulation(bool bCleanup = false);

	UFUNCTION(BlueprintPure)
	bool IsRestoringGameSimulation() const;

	UFUNCTION(BlueprintPure)
	float GetPercent() const;

	UFUNCTION(BlueprintPure)
	int32 GetRestoreSpeed() const;

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface
	
protected:	
	UPROPERTY(Transient)
	TWeakObjectPtr<const UWorld> RestoringWorld;
	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;
	UPROPERTY(Transient)
	TObjectPtr<class UUserWidget> RestoreWidget;

	/* Info about our current restore state (to delete) */
	TSharedPtr<struct FRecallRestoreInfo> RestoreInfo;

	void OnTickSimulation(float DeltaTime);

	/**
	 * Let the host know about this client's current frame.
	 */
	void SendIdleInput(uint32 Frame) const;

	/**
	 * Whether this client can finish restoring.
	 */
	bool CanFinishRestore() const;

	/**
	 * Finish restoring the game simulation.
	 */
	void FinishRestore();

	/**
	 * Get the target frame that the client should reach to finish restoring the game simulation.
	 */
	uint32 GetTargetFrame() const;

	/**
	 * Get the most recent validated target frames.
	 */
	uint32 ValidateTargetFrame(uint32 CurrentFrame, uint32 TargetFrame) const;

	// EasyDataTransfer event handlers
	void OnSnapshotTransferProgressUpdate(int32 Handle, const FString& ChannelName, float Progress);
	void OnSnapshotTransferError(int32 Handle, const FString& ChannelName, EDataTransferError Error);
	
	// Combined restore transfer handlers
	void OnCombinedRestoreDataReceived(int32 Handle, const FString& ChannelName, const TArray<uint8>& Data);
	void ProcessCombinedRestoreData(const TArray<uint8>& Data);
	
	// Helper method to get EasyDataTransfer subsystem
	UEasyDataTransferSubsystem* GetDataTransferSubsystem() const;

};
