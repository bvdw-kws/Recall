// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallSyncReportGameComponent.generated.h"

class ARecallPlayerState_InGame;

/**
 * Component to handle synchronization reports.
 */
UCLASS()
class RECALLONLINE_API URecallSyncReportGameComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	void SignalOutOfSyncPlayer(ARecallPlayerState_InGame* PlayerState);
	
	/** Return true if desync was detected in current game */
	FORCEINLINE bool IsLocalDesyncDetected() const { return bDesyncDetected; }

	uint32 GetDebugSyncedFrame() const;
	void SetDebugSyncedFrame(uint32 Frame);

#pragma region RPC
public:
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_DumpDesyncLog();
#pragma endregion RPC

	//~ Begin UObject Interface.
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface.
	
protected:
	UPROPERTY(EditDefaultsOnly)
	FText OutOfSyncKickReasonText;
	
	UPROPERTY(EditDefaultsOnly, meta=(Units=Seconds))
	float KickOutOfSyncPlayerDelay = 1.0f;
	
	/** true if desync was detected in current game */
	UPROPERTY(Replicated)
	bool bDesyncDetected { false };

	/** 
	* Compare player simulation reports to see if they are in synced.
	* This is a debug feature to keep track of if players are synced or not.
	*/
	UPROPERTY(ReplicatedUsing=OnRep_DebugSyncedFrame)
	uint32 DebugSyncedFrame{ 0 };

	void KickOutOfSyncPlayer(APlayerController* KickedPlayerController);
	
	UFUNCTION()
	void OnRep_DebugSyncedFrame();
};
