// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RecallRestoreTypes.h"
#include "Tickable.h"
#include "Engine/World.h"
#include "DataTransfer/EasyDataTransferTypes.h"

#include "RecallRestoreSubsystem.generated.h"

class ARecallPlayerState_InGame;
class UEasyDataTransferSubsystem;
struct FEasyDataTransferOptions;

UCLASS(config=Game)
class RECALLONLINE_API URecallRestoreSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/* Host start restoring data for this client */
	void OpenRestoreClient(ARecallPlayerState_InGame& PlayerState);

	/* Host stops restoring data for this client */
	void CloseRestoreClient(ARecallPlayerState_InGame& PlayerState);

	/* Host is in the process of restoring data for this client */
	bool IsRestoreClient(ARecallPlayerState_InGame& PlayerState) const;

private:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// FTickableGameObject implementation Begin
	void Tick(float DeltaTime) override final;
	TStatId GetStatId() const override final;
	// FTickableGameObject implementation End
	
private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class UEasyDataTransferSubsystem> DataTransferSubsystem;
	/**
	 * Info about each client restoring, stored by the host.
	 */
	UPROPERTY(Transient)
	TMap<FString, FRecallRestoreClientInfo> ClientRestoreInfos;

	void ServerUpdateClientRestoreData();
	void ServerSendClientRestoreData(ARecallPlayerState_InGame& PlayerState);
	
	void FinishClientRestore(ARecallPlayerState_InGame& PlayerState, uint32 LastRestoreInputFrame);
	
	// Combined snapshot + input transfer
	bool StartCombinedRestoreTransfer(ARecallPlayerState_InGame& PlayerState, FRecallRestoreClientInfo* RestoreClientInfo);

};
