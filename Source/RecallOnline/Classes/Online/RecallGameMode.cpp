// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallGameMode.h"

#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallGameEditorGameComponent.h"
#include "Components/GameState/RecallJoinGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "Data/Player/RecallCharacterDataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/EasyOnlineGameSession.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallHUD_InGame.h"
#include "Online/RecallPawn.h"
#include "Online/RecallPlayerController.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Online/RecallReplaySpectatorPlayerController.h"
#include "Online/RecallSpectatorPawn.h"
#include "System/Restore/RecallRestoreSubsystem.h"
#include "TimerManager.h"
#include "Utility/Game/RecallGameUtils.h"
#include "Utility/Gameplay/RecallGameplayStatics.h"
#include "Utility/Player/RecallPlayerUtils.h"

ARecallGameMode::ARecallGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	GameSessionClass = AEasyOnlineGameSession::StaticClass();
	PlayerControllerClass = ARecallPlayerController::StaticClass();
	GameStateClass = ARecallGameState_InGame::StaticClass();
	PlayerStateClass = ARecallPlayerState_InGame::StaticClass();
	DefaultPawnClass = ARecallPawn::StaticClass();
	SpectatorClass = ARecallSpectatorPawn::StaticClass();
	ReplaySpectatorPlayerControllerClass = ARecallReplaySpectatorPlayerController::StaticClass();
	HUDClass = ARecallHUD_InGame::StaticClass();
}

void ARecallGameMode::InitGameState()
{
	Super::InitGameState();
}

void ARecallGameMode::InitSeamlessTravelPlayer(AController* NewController)
{
	APlayerController* NewPC = Cast<APlayerController>(NewController);

	if (NewPC != nullptr)
	{
		NewPC->PostSeamlessTravel();
		
		SetSeamlessTravelViewTarget(NewPC);

		if (!MustSpectate(NewPC))
		{
			NumPlayers++;
			NumTravellingPlayers--;
		}
	}
	else
	{
		NumBots++;
	}
}

void ARecallGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);
}

void ARecallGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Initialize game rule
	FRecallSimulationStartParams GameSimStartParams;
	GameSimStartParams.GameRuleName = DefaultGameRule;

	URecallGameSimulationComponent* GameSimulationComponent = GetInGameStateChecked()->GetGameSimulationComponentChecked();
	GameSimulationComponent->SetGameSimStartParams(GameSimStartParams);

	RestoreSystem = UWorld::GetSubsystem<URecallRestoreSubsystem>(GetWorld());
	GameEditorComponent = GetInGameStateChecked()->GetGameEditorComponentChecked();
}

void ARecallGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	Recall::Frontend::Utils::RegisterGlobalObserver<IRecallGameReactInterface>(this);
}

void ARecallGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	Recall::Frontend::Utils::UnregisterAllGlobalObservers(this);
}

void ARecallGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSplitscreen();
}

void ARecallGameMode::StartPlay()
{
	// FTimerDelegate OnWaitTravellingPlayersTimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnWaitTravellingPlayersTimerComplete);
	// GetWorldTimerManager().SetTimer(WaitTravellingPlayersTimerHandle, OnWaitTravellingPlayersTimerDelegate, WaitTravellingPlayersDuration, false);

	Super::StartPlay();

	if (GameEditorComponent.IsValid())
	{
		GameEditorComponent->OnStartPlay();
	}
}

void ARecallGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);
	
	// Remove local player two's controller so he cannot join
	if (C->IsLocalPlayerController() == false && PlayerTwoLocalController.IsValid())
	{
		UGameplayStatics::RemovePlayer(PlayerTwoLocalController.Get(), true);
		PlayerTwoLocalController.Reset();
	}

	AssignPlayerId(*C);
}

void ARecallGameMode::SyncReady(AEasyOnlinePlayerController_InGame* PC)
{
	ARecallPlayerControllerBase* PlayerControllerBase = Cast<ARecallPlayerControllerBase>(PC);
	if (!IsValid(PlayerControllerBase) || PlayerControllerBase->IsLocalPlayerController())
	{
		return;
	}
	
	ARecallPlayerState_InGame* PlayerState = PC->GetPlayerState<ARecallPlayerState_InGame>();
	if (!ensureAlways(IsValid(PlayerState)) || PlayerState->HasJoinedGame())
	{
		return;
	}
	
	if (IsMatchInProgress())
	{		
		if (RestoreSystem.IsValid())
		{
			RestoreSystem->OpenRestoreClient(*PlayerState);
		}
		
		PlayerState->Client_RestoreSimulation();
	}
}

void ARecallGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ARecallPlayerState_InGame* ExitingPlayerState = Exiting->GetPlayerState<ARecallPlayerState_InGame>();
	if (!ensureMsgf(IsValid(ExitingPlayerState), TEXT("%hs Exiting Player does not have a valid PS"), __FUNCTION__))
	{
		return;
	}

	if (ExitingPlayerState->IsRestoring() && RestoreSystem.IsValid())
	{
		RestoreSystem->CloseRestoreClient(*ExitingPlayerState);
	}

	GetInGameStateChecked()->GetJoinGameComponentChecked()->LeaveGame(*Exiting);

	// FreePlayerId(*ExitingPlayerState);

	const UWorld* World = GetWorld();
	if (IsValid(World) && !World->bIsTearingDown && !Exiting->IsLocalPlayerController() &&
		FreePlayerIds.Contains(TEXT("Player2")) && ensureMsgf(!PlayerTwoLocalController.IsValid(), TEXT("Should not have a local P2")))
	{
		PlayerTwoLocalController = CastChecked<ARecallPlayerController>(UGameplayStatics::CreatePlayer(World, 1, true));
	}
}

bool ARecallGameMode::ReadyToStartMatch_Implementation()
{
	if (!Super::ReadyToStartMatch_Implementation())
	{
		return false;
	}
	
	// Wait until everyone has finished traveling
	if (NumTravellingPlayers > 0)
	{
		return false;
	}

	int32 NumPlayersExpected = 1;
	int32 NumPlayersSynced = 0;
	GetNumPlayersSyncedAndReady(NumPlayersExpected, NumPlayersSynced);

	if (NumPlayersSynced < NumPlayersExpected)
	{
		return false;
	}
	
	// Wait until the initial delay is elapsed
	if (GetGameTimeSinceCreation() < StartMatchDelay)
	{
		return false;
	}

	if (!GameEditorComponent.IsValid() || !GameEditorComponent->CanStartMatch())
	{
		return false;
	}

	return true;
}

void ARecallGameMode::GetNumPlayersSyncedAndReady(int& OutNumExpected, int& OutNumSynced) const
{
	OutNumExpected = NumPlayers;
	OutNumSynced = 0;

	for (APlayerState* PlayerState : GetInGameStateChecked()->PlayerArray)
	{
		ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlayerState);
		if (IsValid(InGamePlState) && InGamePlState->IsSyncReady())
		{
			OutNumSynced++;
		}
	}
}

void ARecallGameMode::HandleMatchHasStarted()
{	
	Super::HandleMatchHasStarted();
	
	GetWorldTimerManager().ClearTimer(WaitTravellingPlayersTimerHandle);

	/*
	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		// Todo: Handle P1 & P2 setup based on replay
		if (ARecallPlayerController* PlayerOneController = Cast<ARecallPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			PlayerOneController->SetJoinedGame(true);
			PlayerActiveCount++;
		}
		return;
	}
	*/

	LaunchGameSimulation();
}

int32 ARecallGameMode::GetNumPlayers()
{
	int32 Result = Super::GetNumPlayers();

	// Ignore local player two controller if did not join
	if (PlayerTwoLocalController.IsValid() && !PlayerTwoLocalController->HasJoinedGame())
	{
		Result--;
	}

	return Result;
}

void ARecallGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);

	if (ARecallPlayerState_InGame* PlayerState = NewPlayer->GetPlayerState<ARecallPlayerState_InGame>())
	{
		if (ARecallPawn* Pawn = Cast<ARecallPawn>(NewPlayer->GetPawn()))
		{
			Pawn->SetSimPlayerId(PlayerState->GetSimPlayerId());
		}
	}
}

bool ARecallGameMode::CanSpectate_Implementation(APlayerController* Viewer, APlayerState* ViewTarget)
{
	if (const ARecallPlayerState_InGame* PlayerState = Cast<ARecallPlayerState_InGame>(ViewTarget))
	{
		return PlayerState->HasJoinedGame();
	}
	
	return false;
}

bool ARecallGameMode::AllowCheats(APlayerController* P)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	return IsValid(P) && P->HasAuthority();
#else
	return false;
#endif
}

ARecallGameState_InGame* ARecallGameMode::GetInGameStateChecked() const
{
	ARecallGameState_InGame* InGameState = GetGameState<ARecallGameState_InGame>();
	check(InGameState);
	return InGameState;
}

void ARecallGameMode::EndGame(const FString& Reason)
{
	OnGameEnd(Reason);
}

void ARecallGameMode::RestartGame()
{
	if (PlayerTwoLocalController.IsValid())
	{
		UGameplayStatics::RemovePlayer(PlayerTwoLocalController.Get(), true);
		PlayerTwoLocalController.Reset();
	}
	
	Super::RestartGame();
}

bool ARecallGameMode::CanNewPlayerJoinGame() const
{
	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		return false;
	}
	
	URecallJoinGameComponent* JoinGameComponent = GetInGameStateChecked()->GetJoinGameComponentChecked();
	if (JoinGameComponent->GetNumJoinGameSimPlayers() >= MaxPlayingPlayers)
	{
		return false;
	}

	return true;
}

void ARecallGameMode::AssignPlayerId(AController& Controller)
{
	ARecallPlayerState_InGame* NewPlayerState = Controller.GetPlayerState<ARecallPlayerState_InGame>();
	if (ensureMsgf(IsValid(NewPlayerState),
		TEXT("%hs New Player does not have a valid PS"), __FUNCTION__) == false)
	{
		return;
	}

	if (NewPlayerState->GetSimPlayerId().IsEmpty())
	{
		if (Controller.IsPlayerController())
		{
			FString NewPlayerID;
			if (FreePlayerIds.IsEmpty())
			{
				NewPlayerID = Recall::Player::Utils::GetPlayerIdFromIndex(GameSimPlayerIndexGenerator++);
			}
			else
			{
				FreePlayerIds.HeapPop(NewPlayerID);
			}
			
			NewPlayerState->SetSimPlayerId(NewPlayerID);
		}
		else
		{
			const FString NewControllerID = FString::Printf(TEXT("AI-%d"), ++GameSimBotIndexGenerator);
			
			NewPlayerState->SetSimPlayerId(NewControllerID);
		}
	}
	else
	{
		UE_LOG(LogRecallPlayer, Warning,
			TEXT("%hs PlayerId was already assigned"), __FUNCTION__);
	}
}

void ARecallGameMode::FreePlayerId(ARecallPlayerState_InGame& InGamePlayerState)
{
	if (!InGamePlayerState.GetSimPlayerId().IsEmpty())
	{
		FreePlayerIds.Add(InGamePlayerState.GetSimPlayerId());
		InGamePlayerState.ClearSimPlayerId();
	}
}

FRecallPlayerSpawnParameters ARecallGameMode::GetPlayerSpawnParameters(
	const AController& Controller)
{
	const ARecallPlayerState_InGame* PlayerState = Controller.GetPlayerState<ARecallPlayerState_InGame>();
	check(IsValid(PlayerState));
	const FString& PlayerId = PlayerState->GetSimPlayerId();
	checkf(!PlayerId.IsEmpty(), TEXT("PlayerId must be non-empty"));
	
	FRecallPlayerSpawnParameters SpawnParams;
	
	const FSoftObjectPath PlayerEntityConfig = GetPlayerEntityConfig(Controller);
	if (ensureMsgf(!PlayerEntityConfig.IsNull(), TEXT("Missing player entity config")))
	{
		SpawnParams.EntityConfigPath = PlayerEntityConfig.GetAssetPathString();
	}

	SetCustomSpawnParams(PlayerId, SpawnParams.CustomParams);

	return SpawnParams;
}

FSoftObjectPath ARecallGameMode::GetPlayerEntityConfig(const AController& Controller) const
{
	const FName CharacterName = GetPlayerDefaultCharacterName(Controller);
	return GetCharacterEntityConfig(CharacterName);
}

FName ARecallGameMode::GetPlayerDefaultCharacterName(const AController& Controller) const
{
	if (const ARecallPlayerController* PlayerController = Cast<ARecallPlayerController>(&Controller))
	{
		if (ensureAlwaysMsgf(!PlayerController->GetDefaultSimPlayerId().IsEmpty(),
				TEXT("%hs Invalid Player Id"), __FUNCTION__))
		{
			return DefaultPlayerCharacterName;
		}
	}
	else
	{
		return DefaultAICharacterName;
	}
	
	return NAME_None;
}

TArray<FName> ARecallGameMode::GetCharacterNames()
{
	TArray<FName> Results;

#if WITH_EDITOR
	if (PlayableCharacters)
	{
		Results.Append(PlayableCharacters->GetRowNames());
	}
#endif // WITH_EDITOR

	return Results;
}

TArray<FName> ARecallGameMode::GetGameRuleNames()
{
	return Recall::Game::Utils::GetGameRuleNames();
}

FSoftObjectPath ARecallGameMode::GetCharacterEntityConfig(const FName& CharacterName) const
{
	if (ensureMsgf(!CharacterName.IsNone(), TEXT("Character name can not be none")))
	{
		if (ensureMsgf(PlayableCharacters, TEXT("Invalid asset")))
		{
			static const FString ContextString(TEXT("ARecallGameMode::GetPlayerEntityConfig"));

			if (const FRecallCharacterDataTable* CharacterData = PlayableCharacters->FindRow<FRecallCharacterDataTable>(CharacterName, ContextString))
			{
				return CharacterData->EntityConfig;
			}
		}
	}

	return FSoftObjectPath();
}

bool ARecallGameMode::CanJoinGame(ARecallPlayerControllerBase& PlayerController) const
{
	if (!CanNewPlayerJoinGame() || !PlayerController.IsA<ARecallPlayerController>())
	{
		return false;
	}
	
	if (!ensureAlways(!PlayerController.GetDefaultSimPlayerId().IsEmpty()))
	{
		return false;
	}

	return true;
}

void ARecallGameMode::ResetConfirmFrame(uint32 Frame)
{
	URecallSyncInputGameComponent* SyncInputComponent = GetInGameStateChecked()->GetSyncInputComponentChecked();
	SyncInputComponent->ForceConfirmFrame(Frame);
}

TArray<FName> ARecallGameMode::GetAllCharacterNames() const
{
	if (ensureMsgf(PlayableCharacters, TEXT("Invalid asset")))
	{
		return PlayableCharacters->GetRowNames();
	}
	else
	{
		return {};
	}
}

void ARecallGameMode::LaunchGameSimulation()
{
	URecallGameSimulationComponent* GameSimulationComponent = GetInGameStateChecked()->GetGameSimulationComponentChecked();
	GameSimulationComponent->LaunchSimulation();
	
	GetInGameStateChecked()->GetJoinGameComponentChecked()->JoinGameAll();

#if WITH_EDITOR
	PlayerTwoLocalController = Cast<ARecallPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 1));
	if (!PlayerTwoLocalController.IsValid())
	{
		PlayerTwoLocalController = CastChecked<ARecallPlayerController>(
		UGameplayStatics::CreatePlayer(GetWorld(), 1, true));

		if (PlayerTwoLocalController.IsValid() && UGameplayStatics::HasOption(OptionsString, TwoPlayerOption))
		{
			GetInGameStateChecked()->GetJoinGameComponentChecked()->JoinGame(*PlayerTwoLocalController.Get());
		}
	}
#endif // WITH_EDITOR
}

bool ARecallGameMode::CanSaveSnapshot() const
{
	if (IsMatchInProgress() == false)
	{
		return false;
	}

	return true;
}

bool ARecallGameMode::CanLoadSnapshot() const
{
	if (IsMatchInProgress() == false)
	{
		return false;
	}

	return true;
}

void ARecallGameMode::SwapPlayers(AController& OldController, AController& NewController)
{
	const ARecallPlayerState_InGame* OldPlayerState = OldController.GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(OldPlayerState) || !OldPlayerState->HasJoinedGame())
	{
		return;
	}

	const ARecallPlayerState_InGame* NewPlayerState = NewController.GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(NewPlayerState) || NewPlayerState->HasJoinedGame())
	{
		return;
	}

	URecallJoinGameComponent* JoinGameComponent = GetInGameStateChecked()->GetJoinGameComponentChecked();
	JoinGameComponent->LeaveGame(OldController);
	JoinGameComponent->JoinGame(NewController);
}

void ARecallGameMode::OnGameEnd(const FString& Reason)
{
	if (!IsMatchInProgress())
	{
		return;
	}
	
	GetInGameStateChecked()->SetGameEndReason(Reason);
	
	EndMatch();
}

void ARecallGameMode::OnControllerDefeat(const FString& ControllerID, const FString& Reason)
{
	if (!IsMatchInProgress())
	{
		return;
	}
	
	for (APlayerState* PlayerState : GetInGameStateChecked()->PlayerArray)
	{
		ARecallPlayerState_InGame* InGamePlState = Cast<ARecallPlayerState_InGame>(PlayerState);
		if (!IsValid(InGamePlState) || InGamePlState->GetSimPlayerId() != ControllerID)
		{
			continue;
		}

		InGamePlState->SetDefeatReason(Reason);
	}
}

void ARecallGameMode::OnWaitTravellingPlayersTimerComplete()
{
	ReturnToMainMenuHost();	
}

void ARecallGameMode::UpdateSplitscreen() const
{
	const int32 LocalMultiplayer = PlayerTwoLocalController.IsValid() && PlayerTwoLocalController->HasJoinedGame();
	
	UGameplayStatics::SetForceDisableSplitscreen(this, !bAllowSplitscreen || !LocalMultiplayer);
}
