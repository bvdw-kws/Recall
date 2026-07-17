// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSyncInputGameComponent.h"

#include "Components/Controller/RecallInputControllerComponent.h"
#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallSyncReportGameComponent.h"
#include "Components/PlayerState/RecallSyncReportPlayerComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPawn.h"
#include "Online/RecallPlayerController.h"
#include "Online/RecallPlayerState_InGame.h"
#include "System/Synchronization/RecallSynchronizationInterface.h"
#include "Utility/Gameplay/RecallGameplayStatics.h"
#include "Utility/Synchronization/RecallSynchronizationUtils.h"

URecallSyncInputGameComponent::URecallSyncInputGameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	
	SetIsReplicatedByDefault(true);
}

void URecallSyncInputGameComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(URecallSyncInputGameComponent, SharedConfirmFrame);
}

void URecallSyncInputGameComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateLocalConfirmFrame();
	UpdateLeftGamePlayerInputs();
}

void URecallSyncInputGameComponent::UpdateLocalConfirmFrame()
{
	// Do not update the confirmed frame while the match is not in progress.
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	if (!GameState->IsMatchInProgress())
	{
		return;
	}
	
	const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	uint32 NewConfirmFrame = GameSimulationComponent->GetLocalSimulationFrame();

	// While restoring the game simulation, use the server shared confirm frame as the confirmed frame until the client caught up.
	const URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();
	if (ClientRestoreComponent->IsRestoringGameSimulation())
	{
		NewConfirmFrame = SharedConfirmFrame;
	}

	// The confirmed frame is the frame with an input for all the active players.
	for (const APlayerState* PlState : GameState->PlayerArray)
	{
		const ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlState);	
		if (!IsValid(InGamePlState) || !IsConfirmFrameRelevant(*InGamePlState))
		{
			continue;
		}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (InGamePlState->IsLeavingGame())
		{
			UE_LOG(LogRecallInput, Log, TEXT("%hs Waiting until frame %d for leaving %s to be synced"),
				__FUNCTION__, InGamePlState->GetLastReceivedInputFrame(), *InGamePlState->GetSimPlayerId());
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		NewConfirmFrame = FMath::Min(InGamePlState->GetLastReceivedInputFrame(), NewConfirmFrame);
	}

	SetLocalConfirmFrame(NewConfirmFrame);
}

bool URecallSyncInputGameComponent::IsConfirmFrameRelevant(const ARecallPlayerState_InGame& PlayerState) const
{
	if (PlayerState.IsABot() || PlayerState.HasLocalNetOwner() || PlayerState.IsRestoring() || !PlayerState.IsSyncReady())
	{
		return false;
	}

	if (!PlayerState.HasJoinedGame() && !PlayerState.HasAuthority())
	{
		return false;
	}

	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		return false;
	}
	
	return true;
}

void URecallSyncInputGameComponent::SetLocalConfirmFrame(uint32 Frame)
{
	LocalConfirmFrame = Frame;

	bool bDirty = false;

	// Iterate through the local players to check if their confirmed frame has changed.
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	for (APlayerState* PlState : GameState->PlayerArray)
	{
		if (!IsValid(PlState) || !PlState->HasLocalNetOwner() || PlState->IsABot())
		{
			continue;
		}

		ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlState);		
		if (IsValid(InGamePlState) && InGamePlState->GetLocalConfirmFrame() < Frame)
		{
			InGamePlState->SetLocalConfirmFrame(Frame);
			bDirty = true;
		}
	}

	if (bDirty)
	{
		UpdateSharedConfirmFrame();
	}

	// Update the confirmed frame of the game simulation.
	IRecallSynchronizationInterface& Sync = Recall::Synchronization::Utils::GetSynchronizationRef(this);
	Sync.SetConfirmFrame(Frame);
}

void URecallSyncInputGameComponent::SetRemoteConfirmFrameByPlayerSimId(const FString& PlayerId, uint32 Frame)
{
	bool bDirty = false;

	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	for (APlayerState* PlState : GameState->PlayerArray)
	{
		if (!IsValid(PlState) || PlState->HasLocalNetOwner() || PlState->IsABot())
		{
			continue;
		}

		ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlState);
		if (!IsValid(InGamePlState) || InGamePlState->GetSimPlayerId() != PlayerId)
		{
			continue;
		}

		if (InGamePlState->GetLocalConfirmFrame() < Frame)
		{
			InGamePlState->SetLocalConfirmFrame(Frame);
			bDirty = true;
		}
		break;
	}

	if (bDirty)
	{
		UpdateSharedConfirmFrame();
	}
}

void URecallSyncInputGameComponent::UpdateSharedConfirmFrame()
{
	// We use the last synced frame instead of the local confirmed frame because it is used to restore the clients.
	// const IRecallSynchronizationInterface& Sync = Recall::Synchronization::Utils::GetSynchronizationRef(this);
	// const uint32 LastSyncedFramed = Sync.GetLastSyncedFrame();
	
	uint32 NewConfirmFrame = GetLocalConfirmFrame(); // FMath::Max(LastSyncedFramed, GetLocalConfirmFrame());

	// The shared confirmed frame is the lowest confirmed frames of all the players.
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	for (const APlayerState* PlState : GameState->PlayerArray)
	{
		if (!IsValid(PlState) || PlState->IsABot() || PlState->HasLocalNetOwner())
		{
			continue;
		}

		// Ignore restoring players because they are potentially way behind the confirmed frame.
		const ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlState);
		if (!IsValid(InGamePlState) || !InGamePlState->IsSyncReady() || InGamePlState->IsLeavingGame())
		{
			continue;
		}

		NewConfirmFrame = FMath::Min(NewConfirmFrame, InGamePlState->GetLocalConfirmFrame());
	}

	if (GetLocalSharedConfirmFrame() < NewConfirmFrame)
	{
		SetSharedConfirmFrame(NewConfirmFrame);
	}
}

void URecallSyncInputGameComponent::SetSharedConfirmFrame(uint32 Frame)
{
	LocalSharedConfirmFrame = Frame;

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		SharedConfirmFrame = Frame;
	}
#endif // WITH_SERVER_CODE
}

void URecallSyncInputGameComponent::ReceiveInput(const FString& PlayerId, const TArray<FRecallFrameInput>& FrameInputs)
{
	// Push the inputs into the queue before advancing ConfirmFrame, so nothing can ever observe
	// ConfirmFrame promising input for a frame that hasn't been written to the queue yet.
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();

	GameSimulationComponent->PushSimulationInput(PlayerId, FrameInputs);

	ApplyFrameInputsByPlayerId(PlayerId, FrameInputs);
}

void URecallSyncInputGameComponent::ReceiveIdleInput(const FString& PlayerId, uint32 Frame)
{
	ApplyLastReceivedInputFrameByPlayerId(PlayerId, Frame);
}

void URecallSyncInputGameComponent::ApplyFrameInputsByPlayerId(const FString& PlayerId, const TArray<FRecallFrameInput>& FrameInputs)
{
	checkf(FrameInputs.Num() > 0, TEXT("%hs Should send at least one input"), __FUNCTION__);
	const uint32 NewLastReceivedInputFrame = FrameInputs.Last().Frame;

	ApplyLastReceivedInputFrameByPlayerId(PlayerId, NewLastReceivedInputFrame);
}

void URecallSyncInputGameComponent::ApplyLastReceivedInputFrameByPlayerId(const FString& PlayerId,
	uint32 NewLastReceivedInputFrame)
{
	bool bDirty = false;
	
	const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlayerState);		
		if (!IsValid(InGamePlState) || InGamePlState->IsABot())
		{
			continue;
		}

		if (InGamePlState->GetSimPlayerId() != PlayerId)
		{
			continue;
		}
		
		if (InGamePlState->GetLastReceivedInputFrame() < NewLastReceivedInputFrame)
		{
			InGamePlState->SetLastReceivedInputFrame(NewLastReceivedInputFrame);
			bDirty = true;
		}
		break;
	}

	if (bDirty)
	{
		UpdateLocalConfirmFrame();
	}
}

void URecallSyncInputGameComponent::UpdateLeftGamePlayerInputs() const
{
#if WITH_SERVER_CODE
	if (!HasAuthority())
	{
		return;
	}

	ARecallPlayerControllerBase* PC = Cast<ARecallPlayerControllerBase>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC))
	{
		return;
	}
		
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	for (auto It = GameState->PlayerArray.CreateConstIterator(); It; ++It)
	{
		ARecallPlayerState_InGame* InGamePlayerState = Cast<ARecallPlayerState_InGame>(*It);
		if (!IsValid(InGamePlayerState) || InGamePlayerState->IsABot() || !InGamePlayerState->IsLeavingGame())
		{
			continue;
		}

		// Everyone caught up with the inputs of the player that left the game.
		if (InGamePlayerState->GetLastReceivedInputFrame() <= GetLocalSharedConfirmFrame())
		{
			InGamePlayerState->ClearIsLeavingGame();
			continue;
		}

		const FString& PlayerID = InGamePlayerState->GetSimPlayerId();
		const uint32 InputStartFrame = GetLocalSharedConfirmFrame() + 1;
		const uint32 InputEndFrame = InGamePlayerState->GetLastReceivedInputFrame();

		UE_LOG(LogRecallInput, Log, TEXT("%hs Resending leaving %s inputs between frame %d and %d"),
			__FUNCTION__, *PlayerID, InputStartFrame, InputEndFrame);

		// Resend the inputs for the player who left the game until everyone is synced.
		PC->GetGameSimulationInputComponentChecked()->ResendInputs(
			InGamePlayerState->GetSimPlayerId(), InputStartFrame, InputEndFrame);
	}
#endif // WITH_SERVER_CODE
}

void URecallSyncInputGameComponent::ForceConfirmFrame(uint32 Frame)
{
	if (HasAuthority())
	{
		NetMulticast_ForceConfirmFrame(Frame);
	}
	else
	{
		ForceConfirmFrame_Internal(Frame);
	}
}

void URecallSyncInputGameComponent::ForceConfirmFrame_Internal(uint32 Frame)
{
	ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();	
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (ARecallPlayerState_InGame* InGamePlayerState = Cast<ARecallPlayerState_InGame>(PlayerState))
		{
			InGamePlayerState->SetLastReceivedInputFrame(Frame);
			InGamePlayerState->SetLocalConfirmFrame(Frame);
		}
	}

	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (ARecallPlayerControllerBase* PlController = Cast<ARecallPlayerControllerBase>(Iterator->Get()))
			{
				if (ARecallPawn* Pawn = Cast<ARecallPawn>(PlController->GetPawn()))
				{
					Pawn->ClearInputRequests();
				}

				if (ARecallPlayerState_InGame* InGamePlState = PlController->GetPlayerState<ARecallPlayerState_InGame>())
				{
					if (URecallSyncReportPlayerComponent* SyncReportComponent = InGamePlState->GetSyncReportComponent())
					{
						SyncReportComponent->ClearDebugSimulationReport();
					}
				}
			}
		}
	}

	if (HasAuthority())
	{
		URecallSyncReportGameComponent* SyncReportGameComponent = GameState->GetSyncReportComponentChecked();
		SyncReportGameComponent->SetDebugSyncedFrame(Frame);
	}

	LocalConfirmFrame = Frame;
	SetSharedConfirmFrame(Frame);

	if (const TScriptInterface<IRecallSynchronizationInterface> Synchronization = Recall::Synchronization::Utils::GetSynchronization(this))
	{
		Synchronization->SetConfirmFrame(Frame);
	}
}

#pragma region RPC
void URecallSyncInputGameComponent::NetMulticast_ForceConfirmFrame_Implementation(uint32 InConfirmFrame)
{
	ForceConfirmFrame_Internal(InConfirmFrame);
}
#pragma endregion RPC
