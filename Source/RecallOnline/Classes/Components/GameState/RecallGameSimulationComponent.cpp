// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallGameSimulationComponent.h"

#include "Components/Controller/RecallMultiSimControllerComponent.h"
#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallJoinGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "RecallFrontendUtils.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Online/Base/RecallPlayerControllerBase.h"
#include "Online/RecallGameMode.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerState_InGame.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"

template<typename T=APlayerController>
void ForEachPlayerController(const UWorld* World, TFunctionRef< void(T& /*PlayerController*/) > ExecuteFunction)
{
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		T* PC = Cast<T>(*Iterator);
		if (IsValid(PC))
		{
			ExecuteFunction(*PC);
		}
	}
}

URecallGameSimulationComponent::URecallGameSimulationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void URecallGameSimulationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnFrameStartEvent().AddUObject(this, &ThisClass::OnFrameStart);
		SimulationController->GetOnLoadSnapshotEvent().AddUObject(this, &ThisClass::OnLoadSnapshot);
	}

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		WorldStates.GenerateFields(this);
	}
#endif // WITH_SERVER_CODE
}

void URecallGameSimulationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnFrameStartEvent().RemoveAll(this);
		SimulationController->GetOnLoadSnapshotEvent().RemoveAll(this);
	}

	SimulationController = nullptr;
}

void URecallGameSimulationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(URecallGameSimulationComponent, ReplicatedSimulationFrame);
	DOREPLIFETIME(URecallGameSimulationComponent, SimStartParams);
	DOREPLIFETIME(URecallGameSimulationComponent, WorldStates);
	DOREPLIFETIME(URecallGameSimulationComponent, PlayerEvents);
}

void URecallGameSimulationComponent::SetServerSimulationFrame(uint32 Frame)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}

	ReplicatedSimulationFrame = Frame;
#endif // WITH_SERVER_CODE
}

void URecallGameSimulationComponent::SetGameSimStartParams(const FRecallSimulationStartParams& Params)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}
	
	SimStartParams = Params;
#endif // WITH_SERVER_CODE
}

void URecallGameSimulationComponent::ResetPlayerEvents()
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}

	PlayerEvents.ResetPlayerItems();
#endif // WITH_SERVER_CODE
}

void URecallGameSimulationComponent::OnRep_PlayerEvents()
{
}

void URecallGameSimulationComponent::OnFrameStart(uint32 Frame)
{
	// If the frame is being stepped, then the simulation will be at the following frame.
	SetLocalSimulationFrame(Frame + 1);
	
	// We wait until our first frame finished.
	if (Frame == 1 && !bForceRestore)
	{
		ForEachPlayerController<ARecallPlayerControllerBase>(GetWorld(), [](auto& PC)
		{
			PC.OnBeginSimulation();
		});
	}
}

void URecallGameSimulationComponent::OnLoadSnapshot(uint32 Frame)
{
	SetLocalSimulationFrame(Frame);
}

void URecallGameSimulationComponent::SetLocalSimulationFrame(uint32 Frame)
{
	LocalSimulationFrame = Frame;

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		SetServerSimulationFrame(Frame);
	}
#endif // WITH_SERVER_CODE
}

void URecallGameSimulationComponent::LaunchSimulation()
{
	UE_LOG(LogPlayerController, Log, TEXT("%hs"), __FUNCTION__);
	
	checkf(!bForceRestore, TEXT("Why are we restoring?"));

	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();
	ClientRestoreComponent->InitGameSimulation();

	DeferredLaunchSimulation();
}

void URecallGameSimulationComponent::DeferredLaunchSimulation()
{
	if (CanResumeSimulation() == false)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::DeferredLaunchSimulation);
		return;
	}

	LaunchSimulation_Internal();
}

void URecallGameSimulationComponent::LaunchSimulation_Internal()
{
	UE_LOG(LogPlayerController, Log, TEXT("%hs Start simulation"), __FUNCTION__);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->StartSimulation(GetGameSimStartParams(), false);
	}

	DeferredResumeSimulation();
}

bool URecallGameSimulationComponent::CanResumeSimulation() const
{
	ARecallGameState_InGame* InGameState = GetGameStateChecked<ARecallGameState_InGame>();
	for (const APlayerState* PlayerState : InGameState->PlayerArray)
	{
		const ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlayerState);
		if (!IsValid(InGamePlState) || !InGamePlState->HasLocalNetOwner())
		{
			continue;
		}
		
		if (!InGamePlState->HasActorBegunPlay() || InGamePlState->GetSimPlayerId().IsEmpty())
		{
			return false;
		}
	}
	
	return true;
}

void URecallGameSimulationComponent::DeferredResumeSimulation()
{
	if (CanResumeSimulation() == false)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::DeferredResumeSimulation);
		return;
	}

	ForEachPlayerController<ARecallPlayerControllerBase>(GetWorld(), [](auto& PC)
	{
		PC.OnStartSimulation();
	});

	// Event is skipped when restoring.
	if (bForceRestore)
	{
		ForEachPlayerController<ARecallPlayerControllerBase>(GetWorld(), [](auto& PC)
		{
			PC.OnBeginSimulation();
		});
		
		bForceRestore = false;
	}

	ResumeSimulation_Internal();
}

void URecallGameSimulationComponent::ResumeSimulation()
{
	UE_LOG(LogPlayerController, Log, TEXT("%hs"), __FUNCTION__);
	
	DeferredResumeSimulation();
}

void URecallGameSimulationComponent::ResumeSimulation_Internal()
{
	UE_LOG(LogPlayerController, Log, TEXT("%hs"), __FUNCTION__);

	if (IsValid(SimulationController.GetObject()))
	{
		if (SimulationController->IsSimulationPaused())
		{
			SimulationController->ResumeSimulation();
		}
	}
}

void URecallGameSimulationComponent::PauseSimulation()
{
	UE_LOG(LogPlayerController, Log, TEXT("%hs Pause simulation)"), __FUNCTION__);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->PauseSimulation();
	}
}

void URecallGameSimulationComponent::ResetSimulation(bool bRestart /*= true*/, bool bAddPlayers /*= true*/) const
{
	UE_LOG(LogPlayerController, Log, TEXT("%hs Reset simulation)"), __FUNCTION__);

	if (IsValid(SimulationController.GetObject()))
	{
		if (bRestart)
		{
			SimulationController->RestartSimulation(GetGameSimStartParams(), false);
		}
		else
		{
			SimulationController->ResetSimulation();
		}

		if (HasAuthority())
		{
			if (ARecallGameMode* GameMode = GetGameMode<ARecallGameMode>())
			{
				GameMode->ResetConfirmFrame(0);
			}

			if (bAddPlayers)
			{
				const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
				GameState->GetJoinGameComponentChecked()->RespawnPlayerEntities();
			}
		}
		
		ForEachPlayerController<ARecallPlayerControllerBase>(GetWorld(), [](auto& PC)
		{
			if (PC.IsLocalPlayerController())
			{
				PC.GetMultiSimComponentChecked()->GoToWorld(0, true);
			}
		});
	}
}

void URecallGameSimulationComponent::PushSimulationInput(const FString& PlayerId,
	const TArray<FRecallFrameInput>& FrameInputs)
{
	if (IsValid(SimulationController.GetObject()))
	{
		for (const FRecallFrameInput& FrameInput : FrameInputs)
		{
			SimulationController->PushInput(FrameInput.Frame, PlayerId, FrameInput.Input);
		}
	}
}

void URecallGameSimulationComponent::OnRestoreSimulation()
{
	bForceRestore = true;

	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();
	
	// Begin to restore here so we can receive restore data.
	ClientRestoreComponent->RestoreGameSimulation(GetGameSimStartParams(), false);
	
	DeferredRestoreSimulation();
}

void URecallGameSimulationComponent::DeferredRestoreSimulation()
{
	if (CanResumeSimulation() == false)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::DeferredRestoreSimulation);
		return;
	}

	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();
	
	ClientRestoreComponent->StartGameSimulation(GetGameSimStartParams());
}

#pragma region RPC
void URecallGameSimulationComponent::NetMulticast_AddPlayer_Implementation(int32 WorldIndex, uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams)
{
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallSyncInputGameComponent* SyncInputGameComponent = GameState->GetSyncInputComponentChecked();
	
	UE_LOG(LogRecallPlayer, Log,
		TEXT("%hs %s added at frame %d (LocalSimulationFrame: %d, LocalConfirmFrame: %d, LocalSharedConfirmFrame: %d)"),
		__FUNCTION__, *PlayerId, Frame, GetLocalSimulationFrame(), SyncInputGameComponent->GetLocalConfirmFrame(),
		SyncInputGameComponent->GetLocalSharedConfirmFrame());

	ensureAlwaysMsgf(Frame == 0 || Frame > SyncInputGameComponent->GetLocalConfirmFrame(),
		TEXT("%hs New player won't be synced if added before the confirmed frame"), __FUNCTION__);
	
	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->RequestAddPlayer(WorldIndex, Frame, PlayerId, SpawnParams);
	}

	// This player won't send idle inputs once he joined, so this client must adjust his last received input frame locally.
	// If we don't do that, then this client's confirmed frame might get stuck.
	if (Frame > 0)
	{
		SyncInputGameComponent->ApplyLastReceivedInputFrameByPlayerId(PlayerId, Frame - 1);
	}

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		PlayerEvents.PushAddPlayerItem(WorldIndex, Frame, PlayerId, SpawnParams);
		OnRep_PlayerEvents();
	}
#endif // WITH_SERVER_CODE
}

void URecallGameSimulationComponent::NetMulticast_RemovePlayer_Implementation(int32 WorldIndex, uint32 Frame, const FString& PlayerId)
{
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	const URecallSyncInputGameComponent* SyncInputGameComponent = GameState->GetSyncInputComponentChecked();
	
	UE_LOG(LogRecallPlayer, Log, TEXT("%hs %s removed at frame %d (LocalSimulationFrame: %d, LocalConfirmFrame: %d)"),
		__FUNCTION__, *PlayerId, Frame, GetLocalSimulationFrame(), SyncInputGameComponent->GetLocalConfirmFrame());
	
	ensureAlwaysMsgf(Frame == 0 || Frame > SyncInputGameComponent->GetLocalConfirmFrame(),
		TEXT("%hs Old player won't be synced if removed before the confirmed frame"), __FUNCTION__);
	
	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->RequestRemovePlayer(WorldIndex, Frame, PlayerId);
	}

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		PlayerEvents.PushRemovePlayerItem(WorldIndex, Frame, PlayerId);
		OnRep_PlayerEvents();
	}
#endif // WITH_SERVER_CODE
}
#pragma endregion RPC
