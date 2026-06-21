// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallMultiSimControllerComponent.h"

#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/Base/RecallPlayerControllerBase.h"
#include "Online/RecallGameMode.h"
#include "Player/MultiWorldLocalPlayer.h"
#include "System/Snapshot/RecallSnapshotInterface.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Player/RecallPlayerUtils.h"

void URecallMultiSimControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		Recall::Frontend::Utils::GetRef<IRecallSnapshotInterface>(this).GetOnLoadSnapshotEvent().AddUObject(this, &ThisClass::OnLoadSnapshot);
	}
}

void URecallMultiSimControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsLocalController())
	{
		Recall::Frontend::Utils::GetRef<IRecallSnapshotInterface>(this).GetOnLoadSnapshotEvent().RemoveAll(this);
	}
}

void URecallMultiSimControllerComponent::OnLoadSnapshot(uint32 Frame, double TimeSeconds, bool bIsRollback)
{
	if (!bIsRollback)
	{
		if (ARecallGameMode* GameMode = Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->ResetConfirmFrame(Frame > 0 ? Frame - 1 : 0);
		}
	}

	const int32 NewWorldIndex = Recall::Player::Utils::GetPlayerWorldIndex(this, GetDefaultSimPlayerId());
	
	GoToWorld(NewWorldIndex, !bIsRollback);
}

void URecallMultiSimControllerComponent::GoToWorld(int32 WorldIndex, bool bRespawnPlayerCamera /*= false*/)
{
	UWorld* NewWorld = Recall::MultiWorld::Utils::GetWorldByIndex(this, WorldIndex);
	if (!IsValid(NewWorld))
	{
		return;
	}

	if (UMultiWorldLocalPlayer* MultiWorldLocalPlayer = GetPlayer<UMultiWorldLocalPlayer>())
	{
		MultiWorldLocalPlayer->SetCurrentWorld(NewWorld);
	}
}

UWorld* URecallMultiSimControllerComponent::GetCurrentWorld() const
{
	if (const UMultiWorldLocalPlayer* MultiWorldLocalPlayer = GetPlayer<UMultiWorldLocalPlayer>())
	{
		return MultiWorldLocalPlayer->GetCurrentWorld();
	}
	else
	{
		return GetWorld();
	}
}

FString URecallMultiSimControllerComponent::GetDefaultSimPlayerId() const
{
	const ARecallPlayerControllerBase* PlayerController = GetControllerChecked<ARecallPlayerControllerBase>();
	return PlayerController->GetDefaultSimPlayerId();
}

#pragma region RPC
void URecallMultiSimControllerComponent::Client_GoToWorld_Implementation(int32 WorldIndex)
{
	GoToWorld(WorldIndex, true);
}
#pragma endregion RPC
