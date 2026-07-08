// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallCheatManager.h"

#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallJoinGameComponent.h"
#include "Components/GameState/RecallReplayGameComponent.h"
#include "Components/GameState/RecallSyncReportGameComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Online/RecallGameMode.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerController.h"
#include "System/Replay/RecallReplaySubsystem.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Snapshot/RecallSnapshotFileSubsystem.h"

#ifdef WITH_DEBUG_MENU
#include "System/Debug/DebugMenuSubsystem.h"
#endif // WITH_DEBUG_MENU

URecallCheatManager::URecallCheatManager()
	: Super()
{
}

void URecallCheatManager::PostInitProperties()
{
	Super::PostInitProperties();
}

void URecallCheatManager::DumpDesyncLog()
{
#if WITH_SERVER_CODE
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(this)))
	{
		if (!GameState->HasAuthority())
		{
			return;
		}

		GameState->GetSyncReportComponentChecked()->NetMulticast_DumpDesyncLog();
	}
#endif // WITH_SERVER_CODE
}

void URecallCheatManager::LoadReplay(const FString& ReplayName)
{
	if (UWorld* World = GetWorld())
	{
		if (URecallReplaySubsystem* ReplaySustem = UGameInstance::GetSubsystem<URecallReplaySubsystem>(World->GetGameInstance()))
		{
			ReplaySustem->LoadReplay(ReplayName);
		}
	}
}

void URecallCheatManager::SaveReplay(const FString& ReplayName)
{
	if (!ReplayName.IsEmpty())
	{
		if (const UWorld* World = GetWorld())
		{
			if (URecallReplaySubsystem* ReplaySystem = UGameInstance::GetSubsystem<URecallReplaySubsystem>(World->GetGameInstance()))
			{
				ReplaySystem->SaveReplay(ReplayName);
			}
		}
	}
}

void URecallCheatManager::StartReplay()
{
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(this)))
	{
		URecallReplayGameComponent* ReplayComponent = GameState->GetReplayComponentChecked();
		ReplayComponent->StartReplay();
	}
}

void URecallCheatManager::SaveSnapshot(const FString& FileName)
{
	if (FileName.IsEmpty())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (const URecallSnapshotFileSubsystem* SnapshotFileSystem = UGameInstance::GetSubsystem<URecallSnapshotFileSubsystem>(World->GetGameInstance()))
		{
			SnapshotFileSystem->SaveSnapshot(FileName);
		}
	}
}

void URecallCheatManager::LoadSnapshot(const FString& FileName)
{
	if (FileName.IsEmpty())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (URecallSnapshotFileSubsystem* SnapshotFileSystem = UGameInstance::GetSubsystem<URecallSnapshotFileSubsystem>(World->GetGameInstance()))
		{
			SnapshotFileSystem->LoadSnapshot(FileName);
		}
	}
}

void URecallCheatManager::SaveQuickSnapshot()
{
	if (const UWorld* World = GetWorld())
	{
		if (URecallSnapshotFileSubsystem* SnapshotFileSystem = UGameInstance::GetSubsystem<URecallSnapshotFileSubsystem>(World->GetGameInstance()))
		{
			SnapshotFileSystem->SaveQuickSnapshot();
		}
	}
}

void URecallCheatManager::LoadQuickSnapshot()
{
	if (const UWorld* World = GetWorld())
	{
		if (URecallSnapshotFileSubsystem* SnapshotFileSystem = UGameInstance::GetSubsystem<URecallSnapshotFileSubsystem>(World->GetGameInstance()))
		{
			SnapshotFileSystem->LoadQuickSnapshot();
		}
	}
}

void URecallCheatManager::RunAutoSettings(int32 WorkScale /*= 10*/, float CPUMultiplier /*= 1.0f*/, float GPUMultiplier /*= 1.0f*/)
{
	if (GEngine)
	{
		if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
		{
			GameUserSettings->RunHardwareBenchmark(WorkScale, CPUMultiplier, GPUMultiplier);
			GameUserSettings->ApplyHardwareBenchmarkResults();
		}
	}
}

void URecallCheatManager::ResetDebugMenuWindow()
{
#ifdef WITH_DEBUG_MENU
	if (const UWorld* World = GetWorld())
	{
		if (UDebugMenuSubsystem* DebugSystem = UGameInstance::GetSubsystem<UDebugMenuSubsystem>(World->GetGameInstance()))
		{
			DebugSystem->ResetDebugWindow();
		}
	}
#endif // WITH_DEBUG_MENU
}

void URecallCheatManager::AddPlayer()
{
	if (const UWorld* World = GetWorld())
	{
		if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
			UGameplayStatics::GetGameState(this)))
		{
			if (ARecallPlayerController* PlayerController = Cast<ARecallPlayerController>(
				UGameplayStatics::GetPlayerController(World, 1)))
			{
				GameState->GetJoinGameComponentChecked()->JoinGame(*PlayerController, true);
			}
		}
	}
}

void URecallCheatManager::RemovePlayer()
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return;
	}

	ARecallPlayerController* PlayerTwoController = Cast<ARecallPlayerController>(
		UGameplayStatics::GetPlayerController(this, 1));
	if (IsValid(PlayerTwoController) && PlayerTwoController->HasJoinedGame())
	{
		ARecallPlayerController* PlayerOneController = Cast<ARecallPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0));
		if (IsValid(PlayerOneController) && PlayerOneController->HasJoinedGame())
		{
			// Restore Player One and Two pawns
			if (PlayerOneController->GetDefaultSimPlayerId() == PlayerTwoController->GetPawnSimPlayerId())
			{
				APawn* PlayerOnePawn = PlayerOneController->GetPawn();
				APawn* PlayerTwoPawn = PlayerTwoController->GetPawn();

				PlayerTwoController->Possess(PlayerOnePawn);
				PlayerOneController->Possess(PlayerTwoPawn);
			}
		}

		GameState->GetJoinGameComponentChecked()->LeaveGame(*PlayerTwoController);
	}
}

void URecallCheatManager::PossessNextPlayer()
{
	ARecallPlayerController* PlayerOneController = Cast<ARecallPlayerController>(
		UGameplayStatics::GetPlayerController(this, 0));
	if (!IsValid(PlayerOneController) || !PlayerOneController->HasJoinedGame())
	{
		return;
	}

	ARecallPlayerController* PlayerTwoController = Cast<ARecallPlayerController>(
		UGameplayStatics::GetPlayerController(this, 1));
	if (!IsValid(PlayerTwoController) || !PlayerTwoController->HasJoinedGame())
	{
		return;
	}
	
	APawn* PlayerOnePawn = PlayerOneController->GetPawn();
	APawn* PlayerTwoPawn = PlayerTwoController->GetPawn();

	if (IsValid(PlayerOnePawn) && IsValid(PlayerTwoPawn))
	{
		PlayerTwoController->Possess(PlayerOnePawn);
		PlayerOneController->Possess(PlayerTwoPawn);
	}
}

void URecallCheatManager::StepSimulation()
{
	if (URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(GetWorld()))
	{
		MultiSimSystem->SetForceStepSim();
	}
}

void URecallCheatManager::ResetSimulation()
{
	if (ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
		GameSimulationComponent->ResetSimulation();
	}
}

void URecallCheatManager::SetPlayerOneCharacter(const FName& CharacterName)
{
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		GameState->GetJoinGameComponentChecked()->SetLocalPlayerCharacter(CharacterName, 0);
	}
}

void URecallCheatManager::SetPlayerTwoCharacter(const FName& CharacterName)
{
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		GameState->GetJoinGameComponentChecked()->SetLocalPlayerCharacter(CharacterName, 1);
	}
}

void URecallCheatManager::EndGame(const FString& Reason)
{
	if (ARecallGameMode* GameMode = Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->EndGame(Reason);
	}
}
