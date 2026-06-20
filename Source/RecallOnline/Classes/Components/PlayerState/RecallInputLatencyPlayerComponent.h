// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/PlayerStateComponent.h"

#include "RecallInputLatencyPlayerComponent.generated.h"

struct FRecallPlayerInputMetadata;
struct FRecallPlayerLatencyBunch;

/**
 * Component to handle input latency package.
 * These packages are meant to let the client know how much time it took for his input to reach other players.
 * This way he can know the network latency and his frame drift and compensate for it.
 */
UCLASS()
class RECALLONLINE_API URecallInputLatencyPlayerComponent : public UPlayerStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Send back an input latency bunch based on an input.
	 */
	void SendInputLatencyBunchFromInputMetadata(
		ARecallPlayerState_InGame* InputSender, const FRecallPlayerInputMetadata& InputMetadata);
	void GenerateInputMetadata(FRecallPlayerInputMetadata& OutMetadata) const;

#if !UE_BUILD_SHIPPING
	int32 GetReceivedInputBunchCount() const { return ReceivedInputBunchCount; }
	int32 GetReceivedInputLatencyBunchCount() const { return ReceivedInputLatencyBunchCount; }
#endif // !UE_BUILD_SHIPPING

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End UActorComponent Interface

#pragma region RPC
protected:
	/**
	 * The client let the server knows that it received the input with a latency bunch.
	 */
	UFUNCTION(Server, Unreliable)
	void Server_SendInputLatency(ARecallPlayerState_InGame* InputSender, const FRecallPlayerLatencyBunch& LatencyBunch);

	UFUNCTION(Client, Unreliable)
	void Client_SendInputLatency(const FString& PlayerID, const FRecallPlayerLatencyBunch& LatencyBunch);
#pragma endregion RPC
	
private:
#if !UE_BUILD_SHIPPING
	bool bResetInputBunchCounter{ false };
	int32 ReceivedInputBunchCount{ 0 };

	void ProcessResetInputBunchCounter();

	bool bResetInputLatencyBunchCounter{ false };
	int32 ReceivedInputLatencyBunchCount{ 0 };

	void ProcessResetInputLatencyBunchCounter();
#endif // !UE_BUILD_SHIPPING
	
	void ReceiveInputLatencyBunch(const FString& PlayerID, const FRecallPlayerLatencyBunch& LatencyBunch);
	
};
