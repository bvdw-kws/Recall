// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallSyncInputGameComponent.generated.h"

class ARecallPlayerState_InGame;
struct FRecallFrameInput;

/**
 * Component to sync input between players.
 * Also handle local and shared confirmed frame.
 */
UCLASS()
class RECALLONLINE_API URecallSyncInputGameComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	void ReceiveInput(const FString& PlayerId, const TArray<FRecallFrameInput>& FrameInputs);
	void ReceiveIdleInput(const FString& PlayerId, uint32 Frame);

	uint32 GetLocalConfirmFrame() const { return LocalConfirmFrame; }
	void SetLocalConfirmFrame(uint32 Frame);
	void SetRemoteConfirmFrameByPlayerSimId(const FString& PlayerId, uint32 Frame);

	uint32 GetLocalSharedConfirmFrame() const { return LocalSharedConfirmFrame; }
	uint32 GetServerSharedConfirmFrame() const { return SharedConfirmFrame; }

	void ApplyLastReceivedInputFrameByPlayerId(const FString& PlayerId, uint32 NewLastReceivedInputFrame);
	
	void ForceConfirmFrame(uint32 Frame);

	//~ Begin UObject Interface.
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface.
	
	//~ Begin UActorComponent Interface
protected:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End UActorComponent Interface
	
#pragma region RPC
protected:
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_ForceConfirmFrame(uint32 InConfirmFrame);
#pragma endregion RPC

protected:
	/**
	* The lowest confirmed frame among all the synchronized players.
	* This is used to know how far back old inputs should be resent to other players.
	* Restoring players won't be considered for the shared confirm frames,
	* because they receive old inputs directly from the host.
	*/
	UPROPERTY(Replicated)
	uint32 SharedConfirmFrame = 0;

	/**
	 * Confirm frame for this client.
	 * The confirmed frame is the frame for which this client has an input for all the other clients.
	 */
	UPROPERTY(Transient)
	uint32 LocalConfirmFrame = 0;

	/**
	 * Confirm frame validated for all the client.
	 * Inputs are only shared past this frame.
	 */
	UPROPERTY(Transient)
	uint32 LocalSharedConfirmFrame = 0;

	void ApplyFrameInputsByPlayerId(const FString& PlayerId, const TArray<FRecallFrameInput>& FrameInputs);

	/**
	 * Resent inputs for players that left the game until everyone caught up.
	 */
	void UpdateLeftGamePlayerInputs() const;

	/** Update confirm frame, which is the last frame with an input for all the players. */
	void UpdateLocalConfirmFrame();

	/**
	 * Whether this player's input is relevant when updating the confirmed frame.
	 */
	bool IsConfirmFrameRelevant(const ARecallPlayerState_InGame& PlayerState) const;

	void ForceConfirmFrame_Internal(uint32 Frame);

	void UpdateSharedConfirmFrame();
	void SetSharedConfirmFrame(uint32 Frame);
};
