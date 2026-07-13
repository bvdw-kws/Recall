// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallGameState_InGame.h"

#include "Component/EasyOnlineBotCreationComponent.h"
#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallGameEditorGameComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallLatencyGameComponent.h"
#include "Components/GameState/RecallReplayGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "Components/GameState/RecallSyncReportGameComponent.h"
#include "RecallBotController.h"
#include "RecallHUD_InGame.h"
#include "Net/UnrealNetwork.h"
#include "Utility/Online/RecallOnlineUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#include "HAL/IConsoleManager.h"

static bool bDebugShowOnlineInfo = false;
static FAutoConsoleVariableRef CVarRecallShowInfo(
	TEXT("recall.online.ShowInfo"),
	bDebugShowOnlineInfo,
	TEXT("Show Info")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

ARecallGameState_InGame::ARecallGameState_InGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	BotCreationComponent = ObjectInitializer.CreateDefaultSubobject<UEasyOnlineBotCreationComponent>(this, TEXT("BotCreationComponent"));
	if (BotCreationComponent)
	{
		BotCreationComponent->SetBotControllerClass(ARecallBotController::StaticClass());
	}
	
	GameSimulationComponent = ObjectInitializer.CreateDefaultSubobject<URecallGameSimulationComponent>(this, TEXT("GameSimulationComponent"));
	SyncInputComponent = ObjectInitializer.CreateDefaultSubobject<URecallSyncInputGameComponent>(this, TEXT("SyncInputComponent"));
	SyncReportComponent = ObjectInitializer.CreateDefaultSubobject<URecallSyncReportGameComponent>(this, TEXT("SyncReportComponent"));
	ClientRestoreComponent = ObjectInitializer.CreateDefaultSubobject<URecallClientRestoreComponent>(this, TEXT("ClientRestoreComponent"));
	LatencyComponent = ObjectInitializer.CreateDefaultSubobject<URecallLatencyGameComponent>(this, TEXT("LatencyComponent"));
	JoinGameComponent = ObjectInitializer.CreateDefaultSubobject<URecallJoinGameComponent>(this, TEXT("JoinGameComponent"));
	ReplayGameComponent = ObjectInitializer.CreateDefaultSubobject<URecallReplayGameComponent>(this, TEXT("ReplayGameComponent"));
	GameEditorComponent = ObjectInitializer.CreateDefaultSubobject<URecallGameEditorGameComponent>(this, TEXT("GameEditorComponent"));
}

void ARecallGameState_InGame::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARecallGameState_InGame, GameEndReason);
}

void ARecallGameState_InGame::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ARecallGameState_InGame::BeginPlay()
{
	Super::BeginPlay();
}

void ARecallGameState_InGame::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (bDebugShowOnlineInfo)
	{
		Recall::Online::Utils::PrintDebug(this);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void ARecallGameState_InGame::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	if (bPauseGameSimulationOnEndMatch && GameSimulationComponent)
	{
		GameSimulationComponent->PauseSimulation();
	}
}

bool ARecallGameState_InGame::CanLoadReplay() const
{
	return true;
}

void ARecallGameState_InGame::SetGameEndReason(const FString& Reason)
{
	if (!HasAuthority())
	{
		return;
	}
	
	GameEndReason = Reason;
	OnRep_GameEndReason();
}

void ARecallGameState_InGame::OnRep_GameEndReason()
{
	if (const UWorld* World = GetWorld())
	{
		for (auto It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!IsValid(PC) || !PC->IsLocalPlayerController())
			{
				continue;
			}

			if (ARecallHUD_InGame* HUD = Cast<ARecallHUD_InGame>(PC->GetHUD()))
			{
				HUD->OnGameEnd(GameEndReason);
			}
		}
	}
}
