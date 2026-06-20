// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallDebugMenuSubsystem.h"

#include "Async/Async.h"
#include "Debug/DebugMenuInterface.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/RecallPlayerState_InGame.h"
#include "System/Debug/DebugMenuSubsystem.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/Replay/RecallReplayUtils.h"
#include "Utility/Snapshot/RecallSnapshotFileUtils.h"

UE_DISABLE_OPTIMIZATION

URecallDebugMenuSubsystem::URecallDebugMenuSubsystem()
	: Super()
{
}

void URecallDebugMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UDebugMenuSubsystem>();
	
#if WITH_DEBUG_MENU
	DebugMenuSubsystem = UGameInstance::GetSubsystem<UDebugMenuSubsystem>(GetGameInstance());
	if (DebugMenuSubsystem.IsValid())
	{
		CreateDebugMenuItems(DebugMenuSubsystem->GetMutableDebugMenu());

		FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnPostWorldInit);
		FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);
	}
#endif // WITH_DEBUG_MENU
}

void URecallDebugMenuSubsystem::Deinitialize()
{
#if WITH_DEBUG_MENU
	DebugMenuSubsystem.Reset();
	
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
#endif // WITH_DEBUG_MENU
}

void URecallDebugMenuSubsystem::Tick(float DeltaTime)
{
#if WITH_DEBUG_MENU
	QUICK_SCOPE_CYCLE_COUNTER(Recall_DebugMenu_Tick);

	if (!DebugMenuSubsystem.IsValid())
	{
		return;
	}

	if (DebugMenuSubsystem->IsMenuOpened())
	{
		RefreshDebugMenu();
	}
	else
	{
		APlayerController* PlayerOneController = UGameplayStatics::GetPlayerController(this, 0);
		if (IsValid(PlayerOneController))
		{
			CheckPlayerOneShortcuts(PlayerOneController);

			CheckPlayerTwoShortcuts(UGameplayStatics::GetPlayerController(this, 1));
		}
	}	
#endif // WITH_DEBUG_MENU
}

TStatId URecallDebugMenuSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URecallDebugMenuSubsystem, STATGROUP_Tickables);
}

void URecallDebugMenuSubsystem::OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
#if WITH_DEBUG_MENU
	if (World &&
		World == GetWorld() &&
		World->IsGameWorld() &&
		!World->IsPreviewWorld())
	{
	}
#endif // WITH_DEBUG_MENU
}

void URecallDebugMenuSubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
#if WITH_DEBUG_MENU
	if (World &&
		World == GetWorld() &&
		World->IsGameWorld() &&
		!World->IsPreviewWorld())
	{
	}
#endif // WITH_DEBUG_MENU
}

void URecallDebugMenuSubsystem::CheckPlayerOneShortcuts(APlayerController* PlayerController)
{
	check(PlayerController != nullptr);

	if (PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_Special_Left))
	{
		bIsHoldGamepadSpecialLeft = true;
		bIsJustPressedGamepadSpecialLeft = true;
	}
	else if (PlayerController->WasInputKeyJustReleased(EKeys::Gamepad_Special_Left))
	{
		bIsHoldGamepadSpecialLeft = false;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_LeftTrigger))
	{
		bIsHoldGamepadLeftTrigger = true;
	}
	else if (PlayerController->WasInputKeyJustReleased(EKeys::Gamepad_LeftTrigger))
	{
		bIsHoldGamepadLeftTrigger = false;
	}

	// Replay
	if (PlayerController->WasInputKeyJustPressed(EKeys::Four) || 
		PlayerController->WasInputKeyJustPressed(EKeys::NumPadFour) ||
		PlayerController->WasInputKeyJustPressed(EKeys::Insert))
	{
		if (ReplayHandle.IsValid())
		{
			ReplayHandle.Pin()->Exec(PlayerController);
		}
	}

	// Quick Save
	if (PlayerController->WasInputKeyJustPressed(EKeys::Seven) ||
		PlayerController->WasInputKeyJustPressed(EKeys::NumPadSeven) ||
		PlayerController->WasInputKeyJustPressed(EKeys::Home))
	{
		if (QuickSaveHandle.IsValid())
		{
			QuickSaveHandle.Pin()->Exec(PlayerController);
		}
	}

	// Prevent player from using quick load during replay.
	if (PlayerController->WasInputKeyJustPressed(EKeys::Nine) ||
		PlayerController->WasInputKeyJustPressed(EKeys::NumPadNine) ||
		PlayerController->WasInputKeyJustPressed(EKeys::End))
	{
		if (QuickLoadHandle.IsValid())
		{
			QuickLoadHandle.Pin()->Exec(PlayerController);
		}
	}

	// Pause
	if (PlayerController->WasInputKeyJustPressed(EKeys::Zero) ||
		PlayerController->WasInputKeyJustPressed(EKeys::NumPadZero) ||
		PlayerController->WasInputKeyJustPressed(EKeys::PageUp))
	{
		if (PauseHandle.IsValid())
		{
			PauseHandle.Pin()->Exec(PlayerController);
		}
	}

	// Step
	if (PlayerController->WasInputKeyJustPressed(EKeys::Hyphen) ||
		PlayerController->WasInputKeyJustPressed(EKeys::Add) ||
		PlayerController->WasInputKeyJustPressed(EKeys::PageDown))
	{
		if (StepHandle.IsValid())
		{
			StepHandle.Pin()->Exec(PlayerController);
		}
	}

	// Possess P2
	if (IsSimulationPaused() == false)
	{
		if (HasPlayerTwoJoined(true))
		{
			if (PlayerController->WasInputKeyJustPressed(EKeys::NumPadThree) ||
				(bIsHoldGamepadLeftTrigger && bIsJustPressedGamepadSpecialLeft)) // LT + Back
			{
				if (PossessPlayerTwoHandle.IsValid())
				{
					PossessPlayerTwoHandle.Pin()->Exec(PlayerController);
				}

				bIsHoldGamepadSpecialLeft = false;
			}

			if (PlayerController->WasInputKeyJustPressed(EKeys::NumPadOne))
			{
				if (OnePlayerHandle.IsValid())
				{
					OnePlayerHandle.Pin()->Exec(PlayerController);
				}
			}
		}
		else
		{
			if (PlayerController->WasInputKeyJustPressed(EKeys::NumPadTwo))
			{
				if (TwoPlayerHandle.IsValid())
				{
					TwoPlayerHandle.Pin()->Exec(PlayerController);
				}
			}
		}
	}

	// Reset
	if (PlayerController->WasInputKeyJustPressed(EKeys::Comma))
	{
		if (ResetHandle.IsValid())
		{
			ResetHandle.Pin()->Exec(PlayerController);
		}
	}

	bIsJustPressedGamepadSpecialLeft = false;
}

void URecallDebugMenuSubsystem::CheckPlayerTwoShortcuts(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (IsSimulationPaused() == false)
	{
		if (PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftTrigger) &&
			PlayerController->IsInputKeyDown(EKeys::Gamepad_RightTrigger) &&
			PlayerController->IsInputKeyDown(EKeys::Gamepad_Special_Left) &&
			PlayerController->IsInputKeyDown(EKeys::Gamepad_Special_Right))
		{
			if (OnePlayerHandle.IsValid())
			{
				OnePlayerHandle.Pin()->Exec(PlayerController);
			}
		}
	}
}

bool URecallDebugMenuSubsystem::HasPlayerTwoJoined(bool bDebugJoin /*= false*/) const
{
	if (ARecallPlayerState_InGame* PlayerState = Cast<ARecallPlayerState_InGame>(UGameplayStatics::GetPlayerState(GetWorld(), 1)))
	{
		if (PlayerState->HasJoinedGame())
		{
			if (bDebugJoin && !PlayerState->IsDebugJoin())
			{
				return false;
			}

			return true;
		}
	}

	return false;
}

bool URecallDebugMenuSubsystem::IsSimulationPaused() const
{
	TScriptInterface<IRecallSimulationControllerInterface> SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);

	if (SimulationController.GetObject())
	{
		return SimulationController->IsSimulationPaused();
	}

	return false;
}

void URecallDebugMenuSubsystem::RefreshDebugMenu()
{
#if WITH_DEBUG_MENU
	QUICK_SCOPE_CYCLE_COUNTER(Recall_DebugMenu_Refresh);

	if (LoadReplayHandle.IsValid())
	{
		LoadReplayHandle.Pin()->SetList(Recall::Replay::Utils::GetReplayNames());
	}

	if (LoadSnapshotHandle.IsValid())
	{
		FScopeLock Lock(&LoadingSnapshotLock);
		LoadSnapshotHandle.Pin()->SetList(SnapshotFiles);

		if (!bLoadingSnapshotFlag)
		{
			bLoadingSnapshotFlag = true;
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
				{
					FScopeLock Lock(&LoadingSnapshotLock);
					SnapshotFiles = Recall::SnapshotFile::Utils::GetSnapshotFileNames(this);
					bLoadingSnapshotFlag = false;
				}
			);
		}
	}

	/*
	if (PlayerOneCharacterHandle.IsValid())
	{
		PlayerOneCharacterHandle.Pin()->SetList(Recall::DebugMenu::Utils::GetCharacterNames());
	}

	if (PlayerTwoCharacterHandle.IsValid())
	{
		PlayerTwoCharacterHandle.Pin()->SetList(Recall::DebugMenu::Utils::GetCharacterNames());
	}
	*/
#endif // WITH_DEBUG_MENU
}

void URecallDebugMenuSubsystem::CreateDebugMenuItems(IDebugMenu& DebugMenu)
{
#if WITH_DEBUG_MENU
	// AI
	{
		DebugMenu.AddItem_Bool(TEXT("AI"), "State Tree Debug", false, TEXT("recall.AI.DebugStateTreeInWorld"));
		DebugMenu.AddItem_Bool(TEXT("AI"), "Log State Tree To Screen", false, TEXT("recall.AI.DebugStateTreeOnScreen"));
		DebugMenu.AddItem_Bool(TEXT("AI"), "Pause State Tree", false, TEXT("recall.AI.PauseBehaviors"), true);
	}

	// GAME
	{
		DebugMenu.AddItem_InputText(TEXT("Game"), TEXT("EndGame"), TEXT("EndGame"));
	}
	
	// LOCAL
	{
		TwoPlayerHandle = DebugMenu.AddItem_SimpleExec(TEXT("Local"), "Add Player", TEXT("AddPlayer"));
		OnePlayerHandle = DebugMenu.AddItem_SimpleExec(TEXT("Local"), "Remove Player", TEXT("RemovePlayer"));
		PossessPlayerTwoHandle = DebugMenu.AddItem_SimpleExec(TEXT("Local"), "Toggle possess Player Two", TEXT("PossessNextPlayer"));

		// PlayerOneCharacterHandle = DebugMenu.AddItem_List(TEXT("Local"), "Player 1 Character", Recall::DebugMenu::Utils::GetCharacterNames(), TEXT("SetPlayerOneCharacter"));
		// PlayerTwoCharacterHandle = DebugMenu.AddItem_List(TEXT("Local"), "Player 2 Character", Recall::DebugMenu::Utils::GetCharacterNames(), TEXT("SetPlayerTwoCharacter"));
	}

	// PHYSICS
	{
		DebugMenu.AddItem_Bool(TEXT("Physics"), "Show Colliders", false, TEXT("recall.physics.ShowColliders"));
		DebugMenu.AddItem_Bool(TEXT("Physics"), "Show Debug Infos", false, TEXT("recall.physics.ShowDebugInfos"));
		DebugMenu.AddItem_Bool(TEXT("Physics"), "Dump Physics Object", false, TEXT("recall.physics.DumpPhysicsObject"));
	}
	
	// ONLINE
	{
		DebugMenu.AddItem_SimpleExec(TEXT("Online"), "Dump Desync Log", TEXT("DumpDesyncLog"));
		DebugMenu.AddItem_Bool(TEXT("Online"), "Show Info", false, TEXT("recall.online.ShowInfo"));
		DebugMenu.AddItem_Bool(TEXT("Online"), "Break Out Of Sync", false, TEXT("recall.online.BreakOutOfSync"));

		DebugMenu.AddItem_Bool(TEXT("Online"), "Simulation Insight", true, TEXT("recall.simulation.Insight"));

		DebugMenu.AddItem_Bool(TEXT("Online"), "Pause Rollback", false, TEXT("recall.rollback.Pause"));
		DebugMenu.AddItem_Int(TEXT("Online"), "Set Rollback Frames", 0, 0, 8, TEXT("recall.rollback.SetRollbackFrames"));
		DebugMenu.AddItem_Bool(TEXT("Online"), "Random Rollback", false, TEXT("recall.rollback.RandomRollback"));
		DebugMenu.AddItem_Float(TEXT("Online"), "Random Rollback Timer Min", 1.0f, 0.1f, 300.0f, TEXT("recall.rollback.RandomRollbackTimerMin"));
		DebugMenu.AddItem_Float(TEXT("Online"), "Random Rollback Timer Max", 5.0f, 0.1f, 300.0f, TEXT("recall.rollback.RandomRollbackTimerMax"));
		DebugMenu.AddItem_Bool(TEXT("Online"), "Rollback Comparator", false, TEXT("recall.rollback.RollbackComparator"));
		DebugMenu.AddItem_Bool(TEXT("Online"), "Validate Last Synced Frame", false, TEXT("recall.rollback.ValidateLastSyncedFramed"));
	}

	// REPLAY
	{
		ReplayHandle = DebugMenu.AddItem_SimpleExec(TEXT("Replay"), TEXT("Replay"), TEXT("StartReplay"));
		DebugMenu.AddItem_SimpleExec(TEXT("Replay"), TEXT("Instant replay"), TEXT("recall.replay.InstantReplay"));

		DebugMenu.AddItem_Bool(TEXT("Replay"), TEXT("Random Instant Replay"), false, TEXT("recall.replay.RandomInstantReplay"));
		DebugMenu.AddItem_Float(TEXT("Replay"), TEXT("Random Instant Replay Interval"), 5.0f, 0.5f, 60.0f, TEXT("recall.replay.RandomInstantReplayInterval"));

		DebugMenu.AddItem_Bool(TEXT("Replay"), "AutoSave", false, TEXT("recall.replay.AutoSave"));
		LoadReplayHandle = DebugMenu.AddItem_List(TEXT("Replay"), TEXT("Load replay"), Recall::Replay::Utils::GetReplayNames(), TEXT("LoadReplay"));
		DebugMenu.AddItem_InputText(TEXT("Replay"), TEXT("Save replay"), TEXT("SaveReplay"));
	}

	// SIMULATION
	{
		PauseHandle = DebugMenu.AddItem_SimpleExec(TEXT("Simulation"), "Pause", TEXT("recall.simulation.Pause"));
		StepHandle = DebugMenu.AddItem_SimpleExec(TEXT("Simulation"), TEXT("Step"), TEXT("StepSimulation"));
		DebugMenu.AddItem_Float(TEXT("Simulation"), "Speed", 1.0f, 0.1f, 10.0f, TEXT("recall.simulation.Speed"));
		QuickSaveHandle = DebugMenu.AddItem_SimpleExec(TEXT("Simulation"), "Quick Save", TEXT("SaveQuickSnapshot"));
		QuickLoadHandle = DebugMenu.AddItem_SimpleExec(TEXT("Simulation"), "Quick Load", TEXT("LoadQuickSnapshot"));
		DebugMenu.AddItem_InputText(TEXT("Simulation"), TEXT("Save snapshot"), TEXT("SaveSnapshot"));
		LoadSnapshotHandle = DebugMenu.AddItem_List(TEXT("Simulation"), TEXT("Load snapshot"), {}, TEXT("LoadSnapshot"));
		ResetHandle = DebugMenu.AddItem_SimpleExec(TEXT("Simulation"), "Reset", TEXT("ResetSimulation"));
		DebugMenu.AddItem_Bool(TEXT("Simulation"), "Log Snapshot Memory Footprint", false, TEXT("recall.snapshot.LogMemoryFootprint"));

		DebugMenu.AddItem_Bool(TEXT("Simulation"), "Render", true, TEXT("recall.simulation.Render"));
	}
#endif // WITH_DEBUG_MENU
}

UE_ENABLE_OPTIMIZATION
