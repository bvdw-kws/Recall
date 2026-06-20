// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallInputControllerComponent.h"

#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallLatencyGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "Components/Pawn/RecallInputTokenPawnComponent.h"
#include "Components/PlayerState/RecallInputLatencyPlayerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/Base/RecallPlayerControllerBase.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallInputNetSerializers.h"
#include "Online/RecallPawn.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Settings/RecallSimulationSettings.h"
#include "System/Input/RecallInputQueueInterface.h"
#include "Utility/Gameplay/RecallGameplayStatics.h"

URecallInputControllerComponent::URecallInputControllerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecallInputControllerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ping the host while spectating or inactive to let him know which frame we are at.
	if (IsLocalController() && ShouldSendIdleInput())
	{
		SendIdleInput(GetIdleInputFrame());
	}
}

FString URecallInputControllerComponent::GetDefaultSimPlayerId() const
{
	const ARecallPlayerControllerBase* PlayerController = GetControllerChecked<ARecallPlayerControllerBase>();
	return PlayerController->GetDefaultSimPlayerId();
}

void URecallInputControllerComponent::SendIdleInput(uint32 Frame)
{
	checkf(IsLocalController(),
		TEXT("%hs Only local player can sent input latency bunch"), __FUNCTION__);

	ReceiveIdleInput(GetDefaultSimPlayerId(), Frame);

	if (ARecallPlayerState_InGame* InGamePlayerState = GetPlayerState<ARecallPlayerState_InGame>())
	{
		FRecallPlayerInputMetadata InputMetadata;
		InGamePlayerState->GetInputLatencyComponentChecked()->GenerateInputMetadata(InputMetadata);
	
		Server_SendIdleInput(Frame, InputMetadata);
	}
}

void URecallInputControllerComponent::ResendInputs(const FString& PlayerID, uint32 StartFrame, uint32 EndFrame)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}
		
	const auto& InputQueue = Recall::Frontend::Utils::GetRef<IRecallInputQueueInterface>(this);
	
	TArray<FRecallFrameInput> FrameInputs;

	for (uint32 InputFrame = StartFrame; InputFrame <= EndFrame; InputFrame++)
	{
		FRecallInput Input;
		if (InputQueue.GetFrameInput(InputFrame, PlayerID, Input))
		{
			FRecallFrameInput FrameInput;
			FrameInput.Frame = InputFrame;
			FrameInput.Input = Input;
			
			FrameInputs.Add(FrameInput);
		}
	}

	SendInput(PlayerID, FrameInputs);
#endif // WITH_SERVER_CODE
}

void URecallInputControllerComponent::ReceiveIdleInput(
	const FString& PlayerID, uint32 Frame) const
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return;
	}

	URecallSyncInputGameComponent* SyncInputComponent = GameState->GetSyncInputComponentChecked();
	SyncInputComponent->ReceiveIdleInput(PlayerID, Frame);
}

void URecallInputControllerComponent::ReceiveInputBunch(
	const ARecallPawn* InputOwner, const FRecallPlayerInputBunch& InputBunch) const
{
	ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(InGameState))
	{
		return;
	}
	
	URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
		
	if (InputBunch.FrameInputs.Num() && InputBunch.FrameInputs.Last().Frame > SyncInputComponent->GetLocalSharedConfirmFrame())
	{
		if (ARecallPawn* Pawn = GetPawn<ARecallPawn>())
		{
			Pawn->GetInputTokenComponentChecked()->RegenerateInputToken();
		}
	}

	SyncInputComponent->ReceiveInput(InputBunch.PlayerId, InputBunch.FrameInputs);

	SendInputLatencyBunch(InputOwner, InputBunch);
}

void URecallInputControllerComponent::SendInputLatencyBunch(
	const ARecallPawn* InputOwner, const FRecallPlayerInputBunch& InputBunch) const
{
	checkf(IsLocalController(),
		TEXT("%hs Only local player can sent input latency bunch"), __FUNCTION__);
	
	// The owner of the pawn is the player who sent the input.
	ARecallPlayerState_InGame* SenderPS = IsValid(InputOwner) ? InputOwner->GetPlayerState<ARecallPlayerState_InGame>() : nullptr;
	if (!IsValid(SenderPS))
	{
		return;
	}

	if (ARecallPlayerState_InGame* ReceiverPS = GetPlayerState<ARecallPlayerState_InGame>())
	{
		ReceiverPS->GetInputLatencyComponentChecked()->SendInputLatencyBunchFromInputMetadata(SenderPS, InputBunch.Metadata);
	}
}

bool URecallInputControllerComponent::ShouldSendIdleInput() const
{
	// Do not send idle inputs while playing.
	const ARecallPlayerControllerBase* PlayerController = GetController<ARecallPlayerControllerBase>();
	if (!IsValid(PlayerController) || PlayerController->IsInState(NAME_Playing))
	{
		return false;
	}

	// Only send idle inputs while the match is in progress.
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));	
	if (!IsValid(GameState) || !GameState->IsMatchInProgress())
	{
		return false;
	}

	// Wait until this player is synced before sending idle inputs.
	const ARecallPlayerState_InGame* PlayerState = GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(PlayerState) || !PlayerState->IsSyncReady())
	{
		return false;
	}

	// While restoring, let the restore system handle idle inputs.
	const URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();
	if (ClientRestoreComponent->IsRestoringGameSimulation())
	{
		return false;
	}

	return true;
}

uint32 URecallInputControllerComponent::GetIdleInputFrame() const
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return 0;
	}
	
	const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	const uint32 CurrentFrame = GameSimulationComponent->GetLocalSimulationFrame();
	
	const URecallLatencyGameComponent* LatencyGameComponent = GameState->GetLatencyComponentChecked();
	const uint32 InputNetworkLatency = LatencyGameComponent->GetInputNetworkLatencySpikeInFrames();
	
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	
	return CurrentFrame + InputNetworkLatency + SimulationSettings->InputNetworkLatencyBuffer;
}

void URecallInputControllerComponent::SendInput(const FString& PlayerID, const TArray<FRecallFrameInput>& FrameInputs)
{
	if (!ensureAlwaysMsgf(IsLocalController(),
		TEXT("%hs Only local player can sent inputs"), __FUNCTION__))
	{
		return;
	}
	
	const ARecallPlayerState_InGame* InGamePlayerState = GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(InGamePlayerState) || FrameInputs.Num() == 0)
	{
		return;
	}
	
	// Limit how many inputs can be sent at once.
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const int32 InputCount = FMath::Min(SimulationSettings->InputBunchLimit, FrameInputs.Num());
	
	FRecallPlayerInputBunch InputBunch;
	InputBunch.PlayerId = PlayerID;
	InputBunch.FrameInputs = TArray<FRecallFrameInput>(&FrameInputs[0], InputCount);
	
	InGamePlayerState->GetInputLatencyComponentChecked()->GenerateInputMetadata(InputBunch.Metadata);

	Server_SendInput(InputBunch);
}

#pragma region RPC
void URecallInputControllerComponent::Server_SendInput_Implementation(const FRecallPlayerInputBunch& InputBunch)
{
#if WITH_SERVER_CODE
	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		return;
	}
	
	const ARecallPlayerControllerBase* OwnerPlayerController = GetController<ARecallPlayerControllerBase>();
	const ARecallPlayerState_InGame* OwnerPlayerState = GetPlayerState<ARecallPlayerState_InGame>();
	if (!ensure(IsValid(OwnerPlayerController) && IsValid(OwnerPlayerState)))
	{
		return;
	}
	
	// If the sender is leaving the game, then don't send the inputs to other clients.
	if (!OwnerPlayerController->IsLocalPlayerController() && OwnerPlayerState->IsLeavingGame())
	{
		return;
	}
	
	ARecallPawn* OwnerPawn = GetPawn<ARecallPawn>();
	
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			const ARecallPlayerControllerBase* PC = Cast<ARecallPlayerControllerBase>(*Iterator);
			if (!IsValid(PC) || PC == OwnerPlayerController)
			{
				continue;
			}

			const ARecallPlayerState_InGame* PS = PC->GetPlayerState<ARecallPlayerState_InGame>();
			if (!PS->IsSyncReady() || PS->IsRestoring())
			{
				continue;
			}

			PC->GetGameSimulationInputComponentChecked()->Client_SendInput(OwnerPawn, InputBunch);
		}
	}
#endif // WITH_SERVER_CODE
}

void URecallInputControllerComponent::Client_SendInput_Implementation(
	ARecallPawn* InputOwner, const FRecallPlayerInputBunch& InputBunch)
{
	ReceiveInputBunch(InputOwner, InputBunch);
}

void URecallInputControllerComponent::Server_SendIdleInput_Implementation(
	uint32 Frame, const FRecallPlayerInputMetadata& Metadata)
{
#if WITH_SERVER_CODE
	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		return;
	}
	
	const ARecallPlayerControllerBase* OwnerPlayerController = GetController<ARecallPlayerControllerBase>();
	ARecallPlayerState_InGame* OwnerPlayerState = GetPlayerState<ARecallPlayerState_InGame>();
	if (!ensure(IsValid(OwnerPlayerController) && IsValid(OwnerPlayerState)))
	{
		return;
	}

	// Only send idle inputs while a player is inactive or spectating.
	if (!(OwnerPlayerController->IsInState(NAME_Inactive) || OwnerPlayerController->IsInState(NAME_Spectating)))
	{
		return;
	}
	
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(Iterator->Get());
			if (!IsValid(PlayerController) || PlayerController == OwnerPlayerController)
			{
				continue;
			}

			PlayerController->GetGameSimulationInputComponentChecked()->Client_SendIdleInput(
				OwnerPlayerState, Frame, Metadata);
		}
	}
#endif // WITH_SERVER_CODE
}

void URecallInputControllerComponent::Client_SendIdleInput_Implementation(
	ARecallPlayerState_InGame* InputSender, uint32 Frame, const FRecallPlayerInputMetadata& Metadata)
{
	if (!IsValid(InputSender))
	{
		return;
	}
	
	ReceiveIdleInput(InputSender->GetSimPlayerId(), Frame);
	
	if (ARecallPlayerState_InGame* InGamePlayerState = GetPlayerState<ARecallPlayerState_InGame>())
	{
		InGamePlayerState->GetInputLatencyComponentChecked()->SendInputLatencyBunchFromInputMetadata(
			InputSender, Metadata);
	}
}
#pragma endregion RPC
