// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallGameSimViewComponent.h"

#include "RecallFrontendUtils.h"
#include "Online/RecallPawn.h"
#include "Online/RecallPlayerController.h"
#include "Utility/Entity/RecallEntityUtils.h"
#include "Utility/Player/RecallPlayerUtils.h"

URecallGameSimViewComponent::URecallGameSimViewComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URecallGameSimViewComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (const ARecallPawn* Pawn = GetPawn<ARecallPawn>())
	{
		SetGameSimViewTargetControllerID(Pawn->GetSimPlayerId());
	}
	
	Recall::Frontend::Utils::RegisterGlobalObserver<IRecallRepresentationReactInterface>(this);
}

void URecallGameSimViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	Recall::Frontend::Utils::UnregisterAllGlobalObservers(this);
}

void URecallGameSimViewComponent::OnRender()
{
	// This is cached on render to avoid race condition.
	
	const TWeakObjectPtr<AActor> NewGameSimPawn = Recall::Entity::Utils::GetControllerPawn(
		GetWorld(), GetGameSimControllerID());
	GameSimViewTarget = Recall::Entity::Utils::GetControllerViewTarget(
		GetWorld(), GameSimViewTargetControllerID);
	
	if (GameSimPawn.Get() != NewGameSimPawn.Get())
	{
		GameSimPawn = NewGameSimPawn;

		if (ARecallPlayerController* PlayerController = GetController<ARecallPlayerController>())
		{
			PlayerController->ReceiveGameSimPawn();
		}
	}

	GameSimControllerIDs = Recall::Player::Utils::GetPlayersInWorld(GetWorld());
}

FString URecallGameSimViewComponent::GetGameSimControllerID() const
{
	if (const ARecallPawn* Pawn = GetPawn<ARecallPawn>())
	{
		return Pawn->GetSimPlayerId();
	}
	return FString();
}

void URecallGameSimViewComponent::SetGameSimViewTargetControllerID(const FString& ControllerID)
{
	GameSimViewTargetControllerID = ControllerID;
}
