// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerController.h"

#include "Components/GameState/RecallJoinGameComponent.h"
#include "Components/GameState/RecallReplayGameComponent.h"
#include "Curves/CurveFloat.h"
#include "Data/Input/RecallInputActionTableRow.h"
#include "Debug/RecallCheatManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "RecallGameState_InGame.h"
#include "Kismet/GameplayStatics.h"
#include "RecallHUD_InGame.h"
#include "RecallPlayerCameraManager.h"
#include "Online/RecallPawn.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Online/RecallGameMode.h"
#include "System/Debug/DebugMenuSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/Gameplay/RecallGameplayStatics.h"

ARecallPlayerController::ARecallPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(100.0f);

	// We really don't want any input RPC to be dropped
	NetPriority = 100.0f;

	CheatClass = URecallCheatManager::StaticClass();
}

void ARecallPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	DebugMenuSystem = UGameInstance::GetSubsystem<UDebugMenuSubsystem>(GetGameInstance());

	if (DebugMenuSystem.IsValid())
	{
		DebugMenuSystem->OnToggleDebugMenu.AddUObject(this, &ThisClass::OnToggleDebugMenu);
	}
}

void ARecallPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ARecallPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ARecallPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARecallPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocalPlayerController() && IsInState(NAME_Playing) && IsSyncReady())
	{
		if (IsLockInput() == false)
		{
			// Capture mouse position
			if (bUseGameSimulationMousePosition)
			{
				GetMousePosition(CurrentInput.MousePosition.X, CurrentInput.MousePosition.Y);
			}

			PushInput(CurrentInput);

			// Reset options after pushing them
			CurrentInput.ResetOptions();
		}
		else
		{
			// Only push options
			FRecallInput DebugInput;
			DebugInput.Options = CurrentInput.Options;
			PushInput(DebugInput);			
			ClearCurrentInput();
		}
	}
}

void ARecallPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

#if WITH_SERVER_CODE
	if (HasAuthority() && IsLocalPlayerController())
	{
		if (ARecallGameMode* GameMode = Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->AssignPlayerId(*this);
		}
	}
#endif // WITH_SERVER_CODE
}

void ARecallPlayerController::GetSeamlessTravelActorList(bool bToEntry, TArray<class AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToEntry, ActorList);
}

bool ARecallPlayerController::CanRestartPlayer()
{
	if (const ARecallPlayerState_InGame* InGamePlState = GetPlayerState<ARecallPlayerState_InGame>())
	{
		if (InGamePlState->IsSpectator() || !InGamePlState->IsSyncReady() || InGamePlState->IsRestoring())
		{
			return false;
		}
	}
	
	return Super::CanRestartPlayer();
}

FString ARecallPlayerController::GetPawnSimPlayerId() const
{
	if (ARecallPawn* InGamePawn = Cast<ARecallPawn>(GetPawn()))
	{
		return InGamePawn->GetSimPlayerId();
	}

	return FString();
}

#pragma region INPUT
void ARecallPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Movement
		if (MovementAction)
		{
			Input->BindAction(MovementAction, ETriggerEvent::Triggered, this, &ThisClass::OnMovementAction);
			Input->BindAction(MovementAction, ETriggerEvent::Completed, this, &ThisClass::OnMovementAction);
		}

		// Turn At
		if (TurnAtAction)
		{
			Input->BindAction(TurnAtAction, ETriggerEvent::Triggered, this, &ThisClass::OnTurnAtAction);
			Input->BindAction(TurnAtAction, ETriggerEvent::Completed, this, &ThisClass::OnTurnAtAction);
		}
		
		// Join game
		if (JoinAction)
		{
			Input->BindAction(JoinAction, ETriggerEvent::Triggered, this, &ThisClass::OnJoinGameAction);
		}

		// Possess bot		
		if (PossessBotAction)
		{
			Input->BindAction(PossessBotAction, ETriggerEvent::Triggered, this, &ThisClass::OnPossessBotAction);
		}
		
		// Setup input commands
		for (const FName& InputActionName : FRecallInputActionTableRow::GetInputActionNames(InputActionTable))
		{
			const UInputAction* InputAction = FRecallInputActionTableRow::GetInputAction(InputActionTable, InputActionName);
			if (!IsValid(InputAction))
			{
				continue;
			}

			const int32 InputIndex = FRecallInputActionTableRow::GetInputActionIndex(InputActionTable, InputActionName);

			Input->BindAction(InputAction, ETriggerEvent::Triggered, this, &ThisClass::OnActionTriggered, InputIndex);
			Input->BindAction(InputAction, ETriggerEvent::Completed, this, &ThisClass::OnActionCompleted, InputIndex);
		}
	}
}

void ARecallPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();

	if (IsLocalPlayerController())
	{
		if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (PlayingInputMapping)
				{
					InputSystem->AddMappingContext(PlayingInputMapping, 0);
				}
			}
		}
	
		if (ARecallHUD_InGame* HUD = GetHUD<ARecallHUD_InGame>())
		{
			HUD->BeginPlaying();
		}
	}
		
}

void ARecallPlayerController::EndPlayingState()
{
	Super::EndPlayingState();
	
	if (IsLocalPlayerController())
	{
		if (ARecallHUD_InGame* HUD = GetHUD<ARecallHUD_InGame>())
		{
			HUD->EndPlaying();
		}
		
		if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (PlayingInputMapping)
				{
					InputSystem->RemoveMappingContext(PlayingInputMapping);
				}
			}
		}
	}
}

void ARecallPlayerController::OnActionTriggered(const FInputActionValue& Value, int32 InputIndex)
{
	SetInputState(InputIndex, true);

	const uint16 InputCommand = 1 << InputIndex;
	
	switch (Value.GetValueType())
	{
	case EInputActionValueType::Boolean:
		break;
		
	case EInputActionValueType::Axis1D:
		{
			const FInputActionValue::Axis1D Axis1D = Value.Get<FInputActionValue::Axis1D>();
			CurrentInput.AxisInputMap.Add(InputCommand, FVector2D_NetQuantizeDirection(Axis1D, 0.0));
		}
		break;
		
	case EInputActionValueType::Axis2D:
		{			
			const FInputActionValue::Axis2D Axis2D = Value.Get<FInputActionValue::Axis2D>();
			CurrentInput.AxisInputMap.Add(InputCommand, FVector2D_NetQuantizeDirection(Axis2D));
		}
		break;
		
	case EInputActionValueType::Axis3D:
		{			
			const FInputActionValue::Axis3D Axis3D = Value.Get<FInputActionValue::Axis3D>();
			CurrentInput.AxisInputMap.Add(InputCommand, FVector2D_NetQuantizeDirection(Axis3D.X, Axis3D.Y));
		}
		break;

	default:
		unimplemented();
		break;
	}
}

void ARecallPlayerController::OnActionCompleted(const FInputActionValue& Value, int32 InputIndex)
{
	SetInputState(InputIndex, false);
	
	const uint16 InputCommand = 1 << InputIndex;
	
	switch (Value.GetValueType())
	{
	case EInputActionValueType::Boolean:
		break;
		
	case EInputActionValueType::Axis1D:
	case EInputActionValueType::Axis2D:
	case EInputActionValueType::Axis3D:
		CurrentInput.AxisInputMap.Remove(InputCommand);
		break;

	default:
		unimplemented();
		break;
	}
}

void ARecallPlayerController::SetInputState(int32 InputIndex, bool Value)
{
	if (Value)
	{
		CurrentInput.InputCommand |= (1 << InputIndex); // EnumAddFlags
	}
	else
	{
		CurrentInput.InputCommand &= ~(1 << InputIndex); // EnumRemoveFlags
	}
}

void ARecallPlayerController::SetInputOptions(const FString& Options, bool bOverride /*= false*/)
{
	if (bOverride)
	{
		CurrentInput.Options = Options;
	}
	else
	{		
		CurrentInput.Options += Options;
	}
}

const FString& ARecallPlayerController::GetInputOptions() const
{
	return CurrentInput.Options;
}

void ARecallPlayerController::OnMovementAction(const FInputActionValue& Value)
{
	FVector2D AxisValue = Value.Get<FVector2D>();

	if (IsValid(LeftStickRemappingCurve))
	{
		AxisValue.X = LeftStickRemappingCurve->GetFloatValue(FMath::Abs(AxisValue.X)) * FMath::Sign(AxisValue.X);
		AxisValue.Y = LeftStickRemappingCurve->GetFloatValue(FMath::Abs(AxisValue.Y)) * FMath::Sign(AxisValue.Y);
	}

	// Poll Axis Input
	CurrentInput.LeftStickAxis.X = AxisValue.X;
	CurrentInput.LeftStickAxis.Y = AxisValue.Y;
}

void ARecallPlayerController::OnTurnAtAction(const FInputActionValue& Value)
{
	FVector2D AxisValue = Value.Get<FVector2D>();

	// Poll Axis Input
	CurrentInput.RightStickAxis.X = AxisValue.X;
	CurrentInput.RightStickAxis.Y = AxisValue.Y;
}

void ARecallPlayerController::OnJoinGameAction(const FInputActionValue& Value)
{
	JoinGame();
}

void ARecallPlayerController::OnPossessBotAction(const FInputActionValue& Value)
{
	PossessViewTarget();
}

bool ARecallPlayerController::IsLockInput() const
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (DebugMenuSystem.IsValid() && DebugMenuSystem->IsMenuOpened())
	{
		return true;
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	if (URecallGameplayStatics::IsRestoringGameSimulation(this))
	{
		return true;
	}

	if (URecallGameplayStatics::IsPlayingReplay(this))
	{
		return true;
	}

	return false;
}

void ARecallPlayerController::OnToggleDebugMenu(bool bOpen)
{
	if (bOpen)
	{
		// Push empty input so we don't hold the current input.
		if (ARecallPawn* InGamePawn = Cast<ARecallPawn>(GetPawn()))
		{
			InGamePawn->PushInput(FRecallInput());
		}
	}
}

void ARecallPlayerController::PushInput(const FRecallInput& Input)
{
	if (ARecallPawn* InGamePawn = Cast<ARecallPawn>(GetPawn()))
	{
		InGamePawn->PushInput(Input);
	}
}

void ARecallPlayerController::ClearCurrentInput()
{
	CurrentInput = FRecallInput();
}
#pragma endregion INPUT

void ARecallPlayerController::JoinGame()
{
	if (HasAuthority())
	{
		if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
		{
			GameState->GetJoinGameComponentChecked()->JoinGame(*this);
		}
	}
	else
	{
		UE_LOG(LogPlayerController, Log, TEXT("%hs Request JoinGame"), __FUNCTION__);

		Server_JoinGame();
	}
}

void ARecallPlayerController::PossessViewTarget()
{
	if (CanPossessViewTarget())
	{
		Server_PossessViewTarget();
	}
}

bool ARecallPlayerController::CanPossessViewTarget() const
{
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		if (GameState->GetReplayComponentChecked()->IsPlayingReplay())
		{
			return false;
		}
	}
	
	if (const ARecallPawn* ViewTargetAsPawn = Cast<ARecallPawn>(GetViewTarget()))
	{
		const ARecallPlayerState_InGame* ViewTargetPlayerState = ViewTargetAsPawn->GetPlayerState<ARecallPlayerState_InGame>();
		return IsValid(ViewTargetPlayerState) && ViewTargetPlayerState->IsABot();
	}
	return false;
}

void ARecallPlayerController::ReceiveGameSimPawn()
{
	if (ARecallPlayerCameraManager* MSPlayerCameraManager = Cast<ARecallPlayerCameraManager>(PlayerCameraManager))
	{
		MSPlayerCameraManager->OnReceiveSimulationPawn();	
	}
}

#pragma region RPC
void ARecallPlayerController::Server_JoinGame_Implementation()
{
#if WITH_SERVER_CODE
	JoinGame();
#endif // WITH_SERVER_CODE
}

void ARecallPlayerController::Server_PossessViewTarget_Implementation()
{
#if WITH_SERVER_CODE
	if (!IsInState(NAME_Spectating) || !CanPossessViewTarget())
	{
		return;
	}
	
	if (ARecallPawn* BotAsPawn = Cast<ARecallPawn>(GetViewTarget()))
	{
		AController* BotController = BotAsPawn->GetController();
		if (!IsValid(BotController))
		{
			return;
		}
		
		UE_LOG(LogPlayerController, Log, TEXT("%hs Possess Bot: %s"), __FUNCTION__, *BotAsPawn->GetSimPlayerId());

		if (ARecallGameMode* GameMode = Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->SwapPlayers(*BotController, *this);
		}

		Client_PossessViewTarget();
	}
#endif // WITH_SERVER_CODE
}

void ARecallPlayerController::Client_PossessViewTarget_Implementation()
{
	if (ARecallPlayerCameraManager* MSPlayerCameraManager = Cast<ARecallPlayerCameraManager>(PlayerCameraManager))
	{
		MSPlayerCameraManager->OnPossessViewTarget();	
	}
}
#pragma endregion RPC
