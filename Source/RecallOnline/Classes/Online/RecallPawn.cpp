// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPawn.h"

#include "Components/Controller/RecallInputControllerComponent.h"
#include "Components/GameState/RecallLatencyGameComponent.h"
#include "Components/GameState/RecallSyncInputGameComponent.h"
#include "Components/Pawn/RecallGameSimViewComponent.h"
#include "Components/Pawn/RecallInputTokenPawnComponent.h"
#include "Components/Pawn/RecallViewTargetPawnComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RecallGameState_InGame.h"
#include "RecallFrontendUtils.h"
#include "RecallPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "System/Input/RecallInputQueueInterface.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"

ARecallPawn::ARecallPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(100.0f);
	NetPriority = 3.0f;
	
	InputTokenComponent = ObjectInitializer.CreateDefaultSubobject<URecallInputTokenPawnComponent>(
	this, TEXT("InputTokenComponent"));
	GameSimViewComponent = ObjectInitializer.CreateDefaultSubobject<URecallGameSimViewComponent>(
		this, TEXT("GameSimViewComponent"));
	ViewTargetComponent = ObjectInitializer.CreateDefaultSubobject<URecallViewTargetPawnComponent>(
		this, TEXT("ViewTargetComponent"));
}

void ARecallPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);
}

void ARecallPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARecallPawn, SimPlayerId);
}

void ARecallPawn::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnInputPhaseEvent().AddUObject(this, &ThisClass::OnInputPhase);
	}
}

void ARecallPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnInputPhaseEvent().RemoveAll(this);
	}
}

void ARecallPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Re-send last input requests since they are unreliable.
	if (IsOwnedByLocalPlayer())
	{
		ResendInput();
	}	
}

void ARecallPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (ViewTargetComponent)
	{
		ViewTargetComponent->CalcCamera(DeltaTime, OutResult);
	}
	else
	{
		Super::CalcCamera(DeltaTime, OutResult);
	}
}

void ARecallPawn::SetSimPlayerId(const FString& InSimPlayerId, bool bOverride /*= false*/)
{
#if WITH_SERVER_CODE
	if (!ensureAlwaysMsgf(HasAuthority(),
		TEXT("%hs Only the host can set PlayerID"), __FUNCTION__))
	{
		return;
	}
	
	// Only set once.
	if (!bOverride && SimPlayerId.IsEmpty() == false)
	{
		return;
	}

	SimPlayerId = InSimPlayerId;
	OnRep_SimPlayerId();
#endif // WITH_SERVER_CODE
}

void ARecallPawn::OnRep_SimPlayerId()
{
	if (GameSimViewComponent)
	{
		GameSimViewComponent->SetGameSimViewTargetControllerID(SimPlayerId);
	}
}

void ARecallPawn::PushInput(const FRecallInput& Input)
{
	if (PreviousInput == Input)
	{
		return;
	}

	PreviousInput = Input;

	if (InputQueue.IsEmpty())
	{
		InputQueue.Add(Input);
	}
	else
	{
		if (InputQueue.Last().InputCommand != Input.InputCommand)
		{
			InputQueue.Add(Input);
		}
		else
		{
			InputQueue.Last() = Input;
		}
	}
}

void ARecallPawn::OnInputPhase(uint32 Frame, const FString& PlayerId)
{
	if (!IsOwnedByLocalPlayer() || PlayerId != GetSimPlayerId())
	{
		return;
	}

	ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(InGameState))
	{
		return;
	}

	const URecallLatencyGameComponent* LatencyGameComponent = InGameState->GetLatencyComponentChecked();
	const uint32 InputSyncLatency = LatencyGameComponent->EvaluateInputLatencyInFrames();
	const uint32 NextInputFrame = Frame + InputSyncLatency;

	TArray<FRecallFrameInput>& FrameInputs = InputRequest.FrameInputs;

	// We don't want to send new inputs if they were already sent
	if (FrameInputs.Num() && FrameInputs.Last().Frame >= NextInputFrame)
	{
		return;
	}

	FrameInputs.Reset();
	PushLocalFrameInputs(Frame, NextInputFrame, FrameInputs);

	if (FrameInputs.Num())
	{
		UE_LOG(LogPlayerController, Verbose, TEXT("%hs Player<%s> sent input at frame %d for frames %d-%d"), __FUNCTION__,
			*PlayerId, Frame, FrameInputs[0].Frame, FrameInputs[FrameInputs.Num() - 1].Frame)

		// Push inputs and update our local confirmed frame
		URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
		SyncInputComponent->ReceiveInput(SimPlayerId, FrameInputs);

		SendInput(FrameInputs);
	}
}

void ARecallPawn::PushLocalFrameInputs(uint32 Frame, uint32 NextInputFrame, TArray<FRecallFrameInput>& OutFrameInputs)
{
	const ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!ensureMsgf(InGameState,
		TEXT("%hs Invalid game state"), __FUNCTION__))
	{
		return;
	}

	const IRecallInputQueueInterface& SimInputQueue = Recall::Frontend::Utils::GetRef<IRecallInputQueueInterface>(this);

	const URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
	const uint32 ConfirmFrame = SyncInputComponent->GetLocalSharedConfirmFrame();
	const uint32 FirstInputFrame = ConfirmFrame;

	for (uint32 InputFrame = FirstInputFrame; InputFrame <= NextInputFrame; InputFrame++)
	{		
		FRecallInput Input;

		// Resend input whenever possible.
		if (SimInputQueue.GetFrameInput(InputFrame, GetSimPlayerId(), Input))
		{
			FRecallFrameInput OldFrameInput;
			OldFrameInput.Frame = InputFrame;
			OldFrameInput.Input = Input;

			OutFrameInputs.Add(OldFrameInput);
		}
		else
		{
			bool bPopInput = false;
		
			if (InputFrame < NextInputFrame) // We repeat our last input until the next input frame
			{
				Input = LastInput;
			}
			else
			{
				if (InputQueue.IsEmpty())
				{
					Input = LastInput;
				}
				else
				{
					bPopInput = true;
					Input = InputQueue[0];
				}
			}

			FRecallFrameInput NewFrameInput;
			NewFrameInput.Frame = InputFrame;
			NewFrameInput.Input = Input;

			// Compress our input to keep its value consistent between clients
			{
				bool bSuccess = false;

				FBitWriter Writer(0, true);
				NewFrameInput.Input.NetSerialize(Writer, nullptr, bSuccess);

				FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
				NewFrameInput.Input.NetSerialize(Reader, nullptr, bSuccess);
			}

			if (PushLocalInput(NewFrameInput.Frame, NewFrameInput.Input))
			{
				if (bPopInput)
				{
					LastInput = NewFrameInput.Input;
					InputQueue.RemoveAt(0);
				}

				OutFrameInputs.Add(NewFrameInput);
			}
		}
	}
}

bool ARecallPawn::PushLocalInput(uint32 Frame, const FRecallInput& Input) const
{
	if (IsValid(SimulationController.GetObject()))
	{
		FString PlayerId = GetSimPlayerId();
		return SimulationController->TryPushLocalInput(Frame, PlayerId, Input);
	}

	return false;
}

void ARecallPawn::ResendInput()
{
	ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(InGameState))
	{
		return;
	}

	URecallSyncInputGameComponent* SyncInputComponent = InGameState->GetSyncInputComponentChecked();
	
	TArray<FRecallFrameInput>& FrameInputs = InputRequest.FrameInputs;

	// Remove inputs that are behind the confirmed frame
	const uint32 ConfirmFrame = SyncInputComponent->GetLocalSharedConfirmFrame();
	if (ConfirmFrame > 0)
	{
		FrameInputs = FrameInputs.FilterByPredicate([ConfirmFrame](const FRecallFrameInput& FrameInput)
			{
				return FrameInput.Frame > ConfirmFrame;
			}
		);
	}

	if (FrameInputs.Num() && GetInputTokenComponentChecked()->ConsumeInputToken())
	{
		SendInput(FrameInputs);
	}
}

bool ARecallPawn::IsOwnedByLocalPlayer() const
{
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			const ARecallPlayerController* PlayerController = Cast<ARecallPlayerController>(Iterator->Get());
			if (IsValid(PlayerController) && PlayerController->IsLocalPlayerController() &&
				PlayerController->GetDefaultSimPlayerId() == SimPlayerId)
			{
				return true;
			}
		}
	}

	return false;
}

void ARecallPawn::SendInput(const TArray<FRecallFrameInput>& FrameInputs) const
{
	if (const UWorld* World = GetWorld())
	{
		const ARecallPlayerController* PlayerController = Cast<ARecallPlayerController>(
			World->GetFirstPlayerController());
		if (IsValid(PlayerController))
		{
			PlayerController->GetGameSimulationInputComponentChecked()->SendInput(SimPlayerId, FrameInputs);
		}
	}
}
