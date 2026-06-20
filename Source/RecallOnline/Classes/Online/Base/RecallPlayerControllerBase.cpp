// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerControllerBase.h"

#include "Components/Controller/RecallInputControllerComponent.h"
#include "Components/Controller/RecallMultiSimControllerComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallHUD_InGame.h"
#include "Online/RecallPlayerCameraManager.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Online/RecallSpectatorPawn.h"

ARecallPlayerControllerBase::ARecallPlayerControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ARecallPlayerCameraManager::StaticClass();
	
	MultiSimComponent = ObjectInitializer.CreateDefaultSubobject<URecallMultiSimControllerComponent>(
	this, TEXT("MultiSimComponent"));
	GameSimulationInputComponent = ObjectInitializer.CreateDefaultSubobject<URecallInputControllerComponent>(
		this, TEXT("GameSimulationInputComponent"));
}

void ARecallPlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PreviousPlayerAction)
		{
			Input->BindAction(PreviousPlayerAction, ETriggerEvent::Triggered, this, &ThisClass::OnPreviousPlayer);
		}
		
		if (NextPlayerAction)
		{
			Input->BindAction(NextPlayerAction, ETriggerEvent::Triggered, this, &ThisClass::OnNextPlayer);
		}
	}
}

void ARecallPlayerControllerBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Start by being inactive until the game is loaded.
	bPlayerIsWaiting = false;
	StateName = NAME_Inactive;
}

void ARecallPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();
}

void ARecallPlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARecallPlayerControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ARecallPlayerControllerBase::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	if (APawn* P = GetPawnOrSpectator())
	{
		P->CalcCamera(DeltaTime, OutResult);
	}
	else
	{
		Super::CalcCamera(DeltaTime, OutResult);
	}
}

FString ARecallPlayerControllerBase::GetDefaultSimPlayerId() const
{
	if (const ARecallPlayerState_InGame* InGamePlayerState = GetPlayerState<ARecallPlayerState_InGame>())
	{
		return InGamePlayerState->GetSimPlayerId();
	}

	return FString();
}

URecallMultiSimControllerComponent* ARecallPlayerControllerBase::GetMultiSimComponentChecked() const
{
	check(MultiSimComponent);
	return MultiSimComponent;
}

URecallInputControllerComponent* ARecallPlayerControllerBase::GetGameSimulationInputComponentChecked() const
{
	check(GameSimulationInputComponent);
	return GameSimulationInputComponent;
}

bool ARecallPlayerControllerBase::IsPlayerOne() const
{
	return IsLocalPlayerController() && GetLocalPlayer()->GetLocalPlayerIndex() == 0;
}

bool ARecallPlayerControllerBase::HasJoinedGame() const
{
	if (const ARecallPlayerState_InGame* InGamePlayerState = GetPlayerState<ARecallPlayerState_InGame>())
	{
		return InGamePlayerState->HasJoinedGame();
	}
	
	return false;
}

void ARecallPlayerControllerBase::OnStartSimulation()
{
	if (ARecallPlayerCameraManager* MSPlayerCameraManager = Cast<ARecallPlayerCameraManager>(PlayerCameraManager))
	{
		MSPlayerCameraManager->OnStartSimulation();
	}
}

void ARecallPlayerControllerBase::OnBeginSimulation()
{
	if (ARecallPlayerCameraManager* MSPlayerCameraManager = Cast<ARecallPlayerCameraManager>(PlayerCameraManager))
	{
		MSPlayerCameraManager->OnBeginSimulation();
	}
	
	if (IsLocalPlayerController() && IsInState(NAME_Spectating))
	{
		ServerViewNextPlayer();
	}
}

void ARecallPlayerControllerBase::OnPlayerDefeat(const FString& Reason)
{
	if (IsLocalPlayerController() && IsInState(NAME_Playing))
	{
		if (ARecallHUD_InGame* HUD = GetHUD<ARecallHUD_InGame>())
		{
			HUD->OnPlayerDefeat(Reason);
		}
	}
}

#pragma region Spectator
void ARecallPlayerControllerBase::BeginSpectatingState()
{
	Super::BeginSpectatingState();
	
	if (IsLocalPlayerController())
	{
		if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (SpectatorInputMapping)
				{
					InputSystem->AddMappingContext(SpectatorInputMapping, 0);
				}
			}
		}

		if (ARecallHUD_InGame* HUD = GetHUD<ARecallHUD_InGame>())
		{
			HUD->BeginSpectating();
		}
		
		OnNextPlayer();
	}
}

void ARecallPlayerControllerBase::EndSpectatingState()
{
	Super::EndSpectatingState();

	if (IsLocalPlayerController())
	{
		if (ARecallHUD_InGame* HUD = GetHUD<ARecallHUD_InGame>())
		{
			HUD->EndSpectating();
		}
	
		if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (SpectatorInputMapping)
				{
					InputSystem->RemoveMappingContext(SpectatorInputMapping);
				}
			}
		}
	}
}

void ARecallPlayerControllerBase::OnPreviousPlayer()
{
	if (ARecallSpectatorPawn* MSSpectatorPawn = Cast<ARecallSpectatorPawn>(GetSpectatorPawn()))
	{
		MSSpectatorPawn->ViewPrevPlayer();
	}
}

void ARecallPlayerControllerBase::OnNextPlayer()
{
	if (ARecallSpectatorPawn* MSSpectatorPawn = Cast<ARecallSpectatorPawn>(GetSpectatorPawn()))
	{
		MSSpectatorPawn->ViewNextPlayer();
	}
}

void ARecallPlayerControllerBase::RequestSpectate()
{
	Server_RequestSpectate();
}
#pragma endregion Spectator

#pragma region RPC
void ARecallPlayerControllerBase::Client_LaunchGameSimulation_Implementation()
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return;
	}
	
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	GameSimulationComponent->LaunchSimulation();
}

void ARecallPlayerControllerBase::Server_RequestSpectate_Implementation()
{
	if (IsInState(NAME_Spectating))
	{
		return;
	}

	Reset();
	ClientReset();
}

bool ARecallPlayerControllerBase::Server_RequestSpectate_Validate()
{
	return true;
}
#pragma endregion RPC
