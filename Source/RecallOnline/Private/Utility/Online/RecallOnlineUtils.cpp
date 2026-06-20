// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Online/RecallOnlineUtils.h"

#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallLatencyGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "Components/GameState/RecallSyncReportGameComponent.h"
#include "Components/Pawn/RecallInputTokenPawnComponent.h"
#include "Components/PlayerState/RecallInputLatencyPlayerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Online/Base/RecallPlayerControllerBase.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPawn.h"
#include "Online/RecallPlayerState_InGame.h"
#include "RecallFrontendUtils.h"
#include "Settings/RecallSimulationSettings.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

#define OnlineDebugInfoProperty(Name)				DebugString += FString::Format(TEXT("{0}:\t {1} \n"), { #Name });
#define OnlineDebugInfoNamedProperty(Name, Value)	DebugString += FString::Format(TEXT("{0}:\t {1} \n"), { #Name, Value });

namespace Recall::Online::Utils
{

void PrintDebug(const UObject* WorldContextObject)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (!IsValid(WorldContextObject))
	{
		return;
	}

	FString DebugString = TEXT("");

	OnlineDebugInfoNamedProperty(LocalFrame, Recall::Simulation::Utils::GetFrame(WorldContextObject));
	OnlineDebugInfoNamedProperty(LocalTimeSeconds, Recall::Simulation::Utils::GetTimeSeconds(WorldContextObject));

	if (const ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(UGameplayStatics::GetPlayerController(WorldContextObject, 0)))
	{		
		if (const ARecallPlayerState_InGame* PlayerState = PlayerController->GetPlayerState<ARecallPlayerState_InGame>())
		{
			OnlineDebugInfoNamedProperty(SimPlayerId, PlayerState->GetSimPlayerId());

			if (URecallInputLatencyPlayerComponent* InputLatencyComponent = PlayerState->GetInputLatencyComponent())
			{
				OnlineDebugInfoNamedProperty(ReceivedInputBunchCount(local), InputLatencyComponent->GetReceivedInputBunchCount());
				OnlineDebugInfoNamedProperty(ReceivedInputLatencyBunchCount(local), InputLatencyComponent->GetReceivedInputLatencyBunchCount());
			}
		}

		if (const ARecallPawn* Pawn = Cast<ARecallPawn>(PlayerController->GetPawn()))
		{
			OnlineDebugInfoNamedProperty(InputRequestCount(inputs), Pawn->GetInputRequestNum());
			OnlineDebugInfoNamedProperty(InputTokenCount, Pawn->GetInputTokenComponentChecked()->GetInputTokenCount());
		}
	}

	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject)))
	{
		URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
		
		OnlineDebugInfoNamedProperty(ServerFrame, GameSimulationComponent->GetServerSimulationFrame());
		OnlineDebugInfoNamedProperty(NetPause, IsNetPause(GameSimulationComponent) ? TEXT("True") : TEXT("False"));
		
		uint32 RemoteConfirmFrame = 0;
		uint32 LastReceivedInputFrame = 0;
		int32 ReceivedInputBunchCount = 0;
		int32 ReceivedInputLatencyBunchCount = 0;

		for (const APlayerState* RemotePlayerState : GameState->PlayerArray)
		{
			if (!IsValid(RemotePlayerState) || RemotePlayerState->IsInactive() || RemotePlayerState->HasLocalNetOwner()
				|| RemotePlayerState->IsABot())
			{
				continue;
			}

			if (const ARecallPlayerState_InGame* RemoteInGamePlayerState = Cast<ARecallPlayerState_InGame>(RemotePlayerState))
			{
				if (!RemoteInGamePlayerState->IsSyncReady() || RemoteInGamePlayerState->IsRestoring())
				{
					continue;
				}

				if (URecallInputLatencyPlayerComponent* RemoteInputLatencyComponent = RemoteInGamePlayerState->GetInputLatencyComponent())
				{
					ReceivedInputBunchCount = RemoteInputLatencyComponent->GetReceivedInputBunchCount();
					ReceivedInputLatencyBunchCount = RemoteInputLatencyComponent->GetReceivedInputLatencyBunchCount();
				}

				RemoteConfirmFrame = RemoteInGamePlayerState->GetLocalConfirmFrame();
				LastReceivedInputFrame = RemoteInGamePlayerState->GetLastReceivedInputFrame();
				break;
			}
		}

		const URecallSyncInputGameComponent* SyncInputGameComponent = GameState->GetSyncInputComponentChecked();
		const URecallSyncReportGameComponent* SyncReportGameComponent = GameState->GetSyncReportComponentChecked();
		const URecallLatencyGameComponent* LatencyGameComponent = GameState->GetLatencyComponentChecked();
		
		OnlineDebugInfoNamedProperty(InputLatency(frames), LatencyGameComponent->EvaluateInputLatencyInFrames());
		OnlineDebugInfoNamedProperty(SimulationSpeed, LatencyGameComponent->EvaluateSimulationSpeedScale());
		OnlineDebugInfoNamedProperty(NetPauseCounter, LatencyGameComponent->GetNetPauseCounter());
		
		OnlineDebugInfoNamedProperty(FrameDrift, LatencyGameComponent->GetFrameDrift());
		OnlineDebugInfoNamedProperty(NetworkLatencySpike(frames), LatencyGameComponent->GetInputNetworkLatencySpikeInFrames());
		OnlineDebugInfoNamedProperty(ReceivedInputBunchCount(remote), ReceivedInputBunchCount);
		OnlineDebugInfoNamedProperty(ReceivedInputLatencyBunchCount(remote), ReceivedInputLatencyBunchCount);

		OnlineDebugInfoNamedProperty(LocalConfirmFrame, SyncInputGameComponent->GetLocalConfirmFrame());
		OnlineDebugInfoNamedProperty(RemoteConfirmFrame, RemoteConfirmFrame);
		OnlineDebugInfoNamedProperty(LastReceivedInputFrame, LastReceivedInputFrame);
		OnlineDebugInfoNamedProperty(LocalSharedConfirmFrame, SyncInputGameComponent->GetLocalSharedConfirmFrame());
		OnlineDebugInfoNamedProperty(ServerSharedConfirmFrame, SyncInputGameComponent->GetServerSharedConfirmFrame());

		OnlineDebugInfoNamedProperty(ServerDebugSyncedFrame, SyncReportGameComponent->GetDebugSyncedFrame());
	}

	OnlineDebugInfoNamedProperty(NetworkLatency(ms), GetNetworkLatencyInSeconds(WorldContextObject) * 1000.0f);
	OnlineDebugInfoNamedProperty(NetworkLatency(frames), GetNetworkLatencyInFramesCeiled(WorldContextObject));
	OnlineDebugInfoNamedProperty(RoundTripTime(ms), GetRoundTripTimeInSeconds(WorldContextObject) * 1000.0f);

	if (const TScriptInterface<IRecallSimulationControllerInterface> SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(WorldContextObject))
	{
		OnlineDebugInfoNamedProperty(SimControllerElapsedFrame, SimulationController->GetElapsedFrame());
		OnlineDebugInfoNamedProperty(SimControllerElapsedTime, SimulationController->GetElapsedTime());
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		OnlineDebugInfoNamedProperty(WorldRealTimeSeconds, World->GetRealTimeSeconds());
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, DebugString);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

float GetRoundTripTimeInSeconds(const UObject* WorldContextObject)
{
	float PingInMilliseconds = 0.f;

	// Todo: Custom ping calculation for all platforms?
	if (AGameStateBase* GameStateBase = UGameplayStatics::GetGameState(WorldContextObject))
	{
		for (APlayerState* PlayerState : GameStateBase->PlayerArray)
		{
			if (IsValid(PlayerState) == false) continue;
			if (PlayerState->IsInactive()) continue;

			PingInMilliseconds = FMath::Max(PingInMilliseconds, PlayerState->GetPingInMilliseconds());
		}
	}

	return PingInMilliseconds * 0.001f;
}

float GetNetworkLatencyInSeconds(const UObject* WorldContextObject)
{
	const float PingInSeconds = GetRoundTripTimeInSeconds(WorldContextObject);
	const float OneTripPingModifier = 0.5f;
	return PingInSeconds * OneTripPingModifier;
}

int32 GetNetworkLatencyInFramesCeiled(const UObject* WorldContextObject)
{
	const float OneTripPingInSeconds = GetNetworkLatencyInSeconds(WorldContextObject);
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const int32 FramesPerSecond = SimulationSettings->FramesPerSeconds;
	return FMath::CeilToInt(OneTripPingInSeconds * FramesPerSecond);
}

int32 GetNetworkLatencyInFramesRounded(const UObject* WorldContextObject)
{
	const float OneTripPingInSeconds = GetNetworkLatencyInSeconds(WorldContextObject);
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const int32 FramesPerSecond = SimulationSettings->FramesPerSeconds;
	return FMath::RoundToInt(OneTripPingInSeconds * FramesPerSecond);
}

} // namespace Recall::Online::Utils
