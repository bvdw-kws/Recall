// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallInputLatencyPlayerComponent.h"

#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallLatencyGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallInputNetSerializers.h"
#include "Online/RecallPlayerState_InGame.h"

URecallInputLatencyPlayerComponent::URecallInputLatencyPlayerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecallInputLatencyPlayerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URecallInputLatencyPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void URecallInputLatencyPlayerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	ProcessResetInputBunchCounter();
	bResetInputBunchCounter = true;

	ProcessResetInputLatencyBunchCounter();
	bResetInputLatencyBunchCounter = true;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallInputLatencyPlayerComponent::SendInputLatencyBunchFromInputMetadata(
	ARecallPlayerState_InGame* InputSender, const FRecallPlayerInputMetadata& InputMetadata)
{
	ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(InGameState))
	{
		return;
	}
	
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	ProcessResetInputBunchCounter();
	ReceivedInputBunchCount++;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
	SyncInputComponent->SetRemoteConfirmFrameByPlayerSimId(InputSender->GetSimPlayerId(), InputMetadata.LocalConfirmFrame);
		
	const URecallGameSimulationComponent* GameSimulationComponent = InGameState->GetGameSimulationComponentChecked();
	
	FRecallPlayerLatencyBunch LatencyBunch;
	LatencyBunch.InputSentFrame = InputMetadata.LocalFrame;
	LatencyBunch.InputSentRealTimeSeconds = InputMetadata.RealTimeSeconds;
	LatencyBunch.InputReceivedConfirmFrame = SyncInputComponent->GetLocalConfirmFrame();
	LatencyBunch.InputReceivedFrame = GameSimulationComponent->GetLocalSimulationFrame();
	LatencyBunch.bInputReceivedHasAuthority = HasAuthority();	
	
	// Client should use its own controller to send RPC
	Server_SendInputLatency(InputSender, LatencyBunch);
}

void URecallInputLatencyPlayerComponent::GenerateInputMetadata(FRecallPlayerInputMetadata& OutMetadata) const
{	
	if (ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		const URecallGameSimulationComponent* GameSimulationComponent = InGameState->GetGameSimulationComponentChecked();
		OutMetadata.LocalFrame = GameSimulationComponent->GetLocalSimulationFrame();
		
		// Get our new local confirm frame
		const URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
		OutMetadata.LocalConfirmFrame = SyncInputComponent->GetLocalConfirmFrame();
	}

	if (const UWorld* World = GetWorld())
	{
		OutMetadata.RealTimeSeconds = static_cast<float>(World->GetRealTimeSeconds());
	}
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void URecallInputLatencyPlayerComponent::ProcessResetInputBunchCounter()
{
	if (bResetInputBunchCounter)
	{
		ReceivedInputBunchCount = 0;
		bResetInputBunchCounter = false;
	}
}

void URecallInputLatencyPlayerComponent::ProcessResetInputLatencyBunchCounter()
{
	if (bResetInputLatencyBunchCounter)
	{
		ReceivedInputLatencyBunchCount = 0;
		bResetInputLatencyBunchCounter = false;
	}
}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

void URecallInputLatencyPlayerComponent::ReceiveInputLatencyBunch(const FString& PlayerID, const FRecallPlayerLatencyBunch& LatencyBunch)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	ProcessResetInputLatencyBunchCounter();
	ReceivedInputLatencyBunchCount++;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	
	if (ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
		SyncInputComponent->SetRemoteConfirmFrameByPlayerSimId(PlayerID, LatencyBunch.InputReceivedConfirmFrame);
		
		URecallLatencyGameComponent* LatencyGameComponent = InGameState->GetLatencyComponentChecked();
		LatencyGameComponent->ReceiveInputLatencyBunch(LatencyBunch);
	}
}

#pragma region RPC
void URecallInputLatencyPlayerComponent::Server_SendInputLatency_Implementation(
	ARecallPlayerState_InGame* InputSender, const FRecallPlayerLatencyBunch& LatencyBunch)
{
#if WITH_SERVER_CODE
	if (IsValid(InputSender))
	{
		InputSender->GetInputLatencyComponentChecked()->Client_SendInputLatency(InputSender->GetSimPlayerId(), LatencyBunch);
	}
#endif // WITH_SERVER_CODE
}

void URecallInputLatencyPlayerComponent::Client_SendInputLatency_Implementation(const FString& PlayerID, const FRecallPlayerLatencyBunch& LatencyBunch)
{
	ReceiveInputLatencyBunch(PlayerID, LatencyBunch);
}
#pragma endregion RPC
