// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerCameraManager.h"

#include "Components/Pawn/RecallGameSimViewComponent.h"
#include "RecallPawn.h"
#include "RecallPlayerController.h"

ARecallPlayerCameraManager::ARecallPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bClientSimulatingViewTarget = true;
	bDefaultConstrainAspectRatio = true;
}

void ARecallPlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();

	SetManualCameraFade(1.0f, FColor::Black, true);
}

void ARecallPlayerCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARecallPlayerCameraManager::OnStartSimulation()
{
}

void ARecallPlayerCameraManager::OnBeginSimulation()
{
	// For playing controller, wait until they receive their pawn until fading out.
	const ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(GetOwningPlayerController());
	if (!IsValid(PlayerController) || PlayerController->IsInState(NAME_Playing))
	{
		return;
	}
	
	StartCameraFade(1.0f, 0.0f, FadeOutDuration, FColor::Black, true, true);
}

void ARecallPlayerCameraManager::OnPossessViewTarget()
{
	const ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(GetOwningPlayerController());
	if (IsValid(PlayerController))
	{
		// If the pawn is ready, then skip the black screen
		const ARecallPawn* Pawn = PlayerController->GetPawn<ARecallPawn>();
		if (IsValid(Pawn) && IsValid(Pawn->GetGameSimViewComponent()->GetGameSimPawn()))
		{
			return;
		}
	}
	
	SetManualCameraFade(1.0f, FColor::Black, true);
}

void ARecallPlayerCameraManager::OnReceiveSimulationPawn()
{
	const ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(GetOwningPlayerController());
	if (!IsValid(PlayerController) || !PlayerController->IsInState(NAME_Playing))
	{
		return;
	}
	
	StartCameraFade(1.0f, 0.0f, FadeOutDuration, FColor::Black, true, true);
}

void ARecallPlayerCameraManager::OnEnterGameEditor()
{
	StartCameraFade(1.0f, 0.0f, FadeOutDuration, FColor::Black, true, true);
}
