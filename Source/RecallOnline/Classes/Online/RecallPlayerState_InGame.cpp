// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerState_InGame.h"

#include "Base/RecallPlayerControllerBase.h"
#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallPlayerSyncGateComponent.h"
#include "Components/PlayerState/RecallInputLatencyPlayerComponent.h"
#include "Components/PlayerState/RecallSyncReportPlayerComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Online/RecallGameState_InGame.h"

ARecallPlayerState_InGame::ARecallPlayerState_InGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SyncReportComponent = ObjectInitializer.CreateDefaultSubobject<URecallSyncReportPlayerComponent>(
	this, TEXT("SyncReportComponent"));
	InputLatencyComponent = ObjectInitializer.CreateDefaultSubobject<URecallInputLatencyPlayerComponent>(
		this, TEXT("LatencyComponent"));
}

void ARecallPlayerState_InGame::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARecallPlayerState_InGame, SimPlayerId);
	DOREPLIFETIME(ARecallPlayerState_InGame, bJoinedGame);
	DOREPLIFETIME(ARecallPlayerState_InGame, bIsRestoring);
	DOREPLIFETIME(ARecallPlayerState_InGame, RestoreTargetFrame);
	DOREPLIFETIME(ARecallPlayerState_InGame, bIsLeavingGame);
	DOREPLIFETIME(ARecallPlayerState_InGame, DefeatReason);
}

void ARecallPlayerState_InGame::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ARecallPlayerState_InGame::OnDeactivated()
{
	// Keep this player state active until we finish syncing the remaining inputs for this client.
	if (IsLeavingGame())
	{
		bWaitLeavingOnDeactivated = true;
		return;
	}

	Super::OnDeactivated();
}

void ARecallPlayerState_InGame::SetSimPlayerId(const FString& InSimPlayerId)
{ 
	checkf(HasAuthority(), TEXT("Only host can set player ID"));

	if (InSimPlayerId.IsEmpty() || !SimPlayerId.IsEmpty())
	{
		return;
	}

	SimPlayerId = InSimPlayerId; 
}

void ARecallPlayerState_InGame::ClearSimPlayerId()
{
	checkf(HasAuthority(), TEXT("Only host can set player ID"));
	
	SimPlayerId = FString();
}

void ARecallPlayerState_InGame::SetJoinedGame(bool bInJoinedGame, TOptional<uint32> Frame /*= {}*/, bool bInDebugJoin /*= false*/)
{
	checkf(HasAuthority(), TEXT("Only host can modify this variable from GameMode"));
	
	if (bJoinedGame == bInJoinedGame)
	{
		return;
	}

	bJoinedGame = bInJoinedGame;
	bDebugJoin = bInDebugJoin;

#if WITH_SERVER_CODE
	const uint32 EventFrame = Frame.IsSet() ? Frame.GetValue() : 0;
	URecallPlayerSyncGateComponent::GetRef(this).ServerPushEvent(EventFrame);
#endif // WITH_SERVER_CODE

	OnRep_JoinedGame();
}

void ARecallPlayerState_InGame::OnRep_JoinedGame()
{
	if (ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		if (HasLocalNetOwner())
		{
			if (bJoinedGame)
			{
				InGameState->OnLocalJoinGame.Broadcast(this);
			}
			else
			{
				InGameState->OnLocalLeaveGame.Broadcast(this);
			}
		}
		
		URecallPlayerSyncGateComponent* PlayerSyncGateComponent = InGameState->GetPlayerSyncGateComponentChecked();
		PlayerSyncGateComponent->ApplyFlagEvent();
	}
}

void ARecallPlayerState_InGame::SetLastReceivedInputFrame(uint32 Frame)
{
	LastReceivedInputFrame = Frame;
}

uint32 ARecallPlayerState_InGame::GetLastReceivedInputFrame() const
{
	return LastReceivedInputFrame;
}

void ARecallPlayerState_InGame::SetLocalConfirmFrame(uint32 Frame)
{
	LocalConfirmFrame = Frame;
}

uint32 ARecallPlayerState_InGame::GetLocalConfirmFrame() const
{
	return LocalConfirmFrame;
}

void ARecallPlayerState_InGame::SetIsRestoring(bool bInRestoring)
{
	check(HasAuthority());
	bIsRestoring = bInRestoring;
}

void ARecallPlayerState_InGame::SetRestoreTargetFrame(uint32 Frame)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}
	
	RestoreTargetFrame = Frame;
#endif // WITH_SERVER_CODE
}

uint32 ARecallPlayerState_InGame::GetRestoreTargetFrame() const
{
	return RestoreTargetFrame;
}

void ARecallPlayerState_InGame::SetIsLeavingGame(TOptional<uint32> Frame /*= {}*/)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}

	bIsLeavingGame = true;
	OnRep_IsLeavingGame();
#endif // WITH_SERVER_CODE
}

void ARecallPlayerState_InGame::OnRep_IsLeavingGame()
{
}

void ARecallPlayerState_InGame::ClearIsLeavingGame()
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}
	
	bIsLeavingGame = false;
	OnRep_IsLeavingGame();

	if (bWaitLeavingOnDeactivated)
	{
		OnDeactivated();
		bWaitLeavingOnDeactivated = false;
	}
#endif // WITH_SERVER_CODE
}

bool ARecallPlayerState_InGame::IsLeavingGame() const
{
	return bIsLeavingGame;
}

void ARecallPlayerState_InGame::SetDefeatReason(const FString& Reason)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()) || DefeatReason == Reason)
	{
		return;
	}

	DefeatReason = Reason;
	OnRep_DefeatReason();
#endif // WITH_SERVER_CODE
}

void ARecallPlayerState_InGame::OnRep_DefeatReason()
{
	ARecallPlayerControllerBase* PC = Cast<ARecallPlayerControllerBase>(GetPlayerController());
	if (IsValid(PC))
	{
		PC->OnPlayerDefeat(DefeatReason);
	}	
}

#pragma region RPC
void ARecallPlayerState_InGame::Client_RestoreSimulation_Implementation()
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return;
	}
	
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	GameSimulationComponent->OnRestoreSimulation();
}
#pragma endregion RPC
