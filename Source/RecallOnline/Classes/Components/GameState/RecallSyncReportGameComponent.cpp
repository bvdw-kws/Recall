// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "RecallSyncReportGameComponent.h"

#include "RecallFrontendUtils.h"
#include "GameFramework/GameSession.h"
#include "Net/UnrealNetwork.h"
#include "Online/RecallPlayerState_InGame.h"
#include "System/Desync/RecallDesyncLogInterface.h"
#include "System/Simulation/Insight/RecallSimulationInsightInterface.h"

#define LOCTEXT_NAMESPACE "URecallSyncReportGameComponent"

URecallSyncReportGameComponent::URecallSyncReportGameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	
	OutOfSyncKickReasonText = NSLOCTEXT("URecallSyncReportGameComponent", "OutOfSyncKickReasonText", "Out of sync");
}

void URecallSyncReportGameComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(URecallSyncReportGameComponent, bDesyncDetected);
	DOREPLIFETIME(URecallSyncReportGameComponent, DebugSyncedFrame);
}

void URecallSyncReportGameComponent::SignalOutOfSyncPlayer(ARecallPlayerState_InGame* PlayerState)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()) || bDesyncDetected)
	{
		return;
	}
	
	NetMulticast_DumpDesyncLog();
	bDesyncDetected = true;

	// Kick the out-of-sync player, but wait a bit to give him time to dump his de-sync report.
	FTimerHandle TimerHandle;	
	TWeakObjectPtr<APlayerController> WeakKickedPlayerController = IsValid(PlayerState) ? PlayerState->GetPlayerController() : nullptr;
	GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this,
[this, WeakKickedPlayerController]()
		{
			if (!WeakKickedPlayerController.IsValid())
			{
				return;
			}

			KickOutOfSyncPlayer(WeakKickedPlayerController.Get());
		}
	), KickOutOfSyncPlayerDelay, false);
#endif // WITH_SERVER_CODE
}

void URecallSyncReportGameComponent::KickOutOfSyncPlayer(APlayerController* KickedPlayerController)
{
#if WITH_SERVER_CODE	
	if (AGameModeBase* GameMode = GetGameMode<AGameModeBase>())
	{
		if (AGameSession* GameSession = GameMode->GameSession)
		{
			GameSession->KickPlayer(KickedPlayerController, OutOfSyncKickReasonText);
		}
	}

	bDesyncDetected = false;
#endif // WITH_SERVER_CODE
}

uint32 URecallSyncReportGameComponent::GetDebugSyncedFrame() const
{
	return DebugSyncedFrame;
}

void URecallSyncReportGameComponent::SetDebugSyncedFrame(uint32 Frame)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}

	DebugSyncedFrame = Frame;
	OnRep_DebugSyncedFrame();
#endif // WITH_SERVER_CODE
}

void URecallSyncReportGameComponent::OnRep_DebugSyncedFrame()
{
	const TScriptInterface<IRecallSimulationInsightInterface> Insight = Recall::Frontend::Utils::Get<IRecallSimulationInsightInterface>(this);
	if (Insight)
	{
		Insight->SetReportFromFrame(DebugSyncedFrame);
	}
}

#pragma region RPC
void URecallSyncReportGameComponent::NetMulticast_DumpDesyncLog_Implementation()
{
	const TScriptInterface<IRecallDesyncLogInterface> DesyncLog = Recall::Frontend::Utils::Get<IRecallDesyncLogInterface>(this);
	if (DesyncLog)
	{
		DesyncLog->DumpDesyncLog();
	}
}
#pragma endregion RPC

#undef LOCTEXT_NAMESPACE
