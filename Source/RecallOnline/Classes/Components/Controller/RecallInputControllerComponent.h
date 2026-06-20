// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/ControllerComponent.h"

#include "RecallInputControllerComponent.generated.h"

class ARecallPawn;
class ARecallPlayerState_InGame;
struct FRecallFrameInput;
struct FRecallPlayerInputBunch;
struct FRecallPlayerInputMetadata;

/**
 * Component to dispatch inputs for the game simulation.
 */
UCLASS()
class RECALLONLINE_API URecallInputControllerComponent : public UControllerComponent
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Send an idle input to the other clients so they know at which frame we are at.
	 */
	void SendIdleInput(uint32 Frame);

	/**
	 * Resent inputs for a given player.
	 */
	void ResendInputs(const FString& PlayerID, uint32 StartFrame, uint32 EndFrame);
	
	void SendInput(const FString& PlayerID, const TArray<FRecallFrameInput>& FrameInputs);
	void ReceiveInputBunch(const ARecallPawn* InputOwner, const FRecallPlayerInputBunch& InputBunch) const;
	
#pragma region RPC
public:
	/**
	 * Dispatch player input to all the players.
	 * This is done through this controller so even spectator players can receive inputs.
	 */
	UFUNCTION(Client, Unreliable)
	void Client_SendInput(ARecallPawn* InputOwner, const FRecallPlayerInputBunch& InputBunch);

protected:
	/**
	 * Send high priority input to the server so it can be dispatched to all the other clients.
	 */
	UFUNCTION(Server, Unreliable)
	void Server_SendInput(const FRecallPlayerInputBunch& InputBunch);
	
	/**
	 * Send this client's current frame to let the other clients know where he is at.
	 */
	UFUNCTION(Server, Unreliable)
	void Server_SendIdleInput(uint32 Frame, const FRecallPlayerInputMetadata& Metadata);	

	UFUNCTION(Client, Unreliable)
	void Client_SendIdleInput(ARecallPlayerState_InGame* InputSender, uint32 Frame, const FRecallPlayerInputMetadata& Metadata);
#pragma endregion RPC

	//~ Begin UActorComponent Interface
protected:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End UActorComponent Interface
	
protected:
	FString GetDefaultSimPlayerId() const;
	
private:
	void ReceiveIdleInput(const FString& PlayerID, uint32 Frame) const;
	void SendInputLatencyBunch(const ARecallPawn* InputOwner, const FRecallPlayerInputBunch& InputBunch) const;
	
	bool ShouldSendIdleInput() const;
	uint32 GetIdleInputFrame() const;
};
