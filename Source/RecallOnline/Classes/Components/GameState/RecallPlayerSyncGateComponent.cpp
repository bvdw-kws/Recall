// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerSyncGateComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Online/RecallGameState_InGame.h"

URecallPlayerSyncGateComponent::URecallPlayerSyncGateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void URecallPlayerSyncGateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URecallPlayerSyncGateComponent, ReplicatedEventCount);
}

URecallPlayerSyncGateComponent& URecallPlayerSyncGateComponent::GetRef(const UObject* WorldContextObject)
{
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject)))
	{
		return *GameState->GetPlayerSyncGateComponentChecked();
	}

	checkNoEntry();
	return *NewObject<URecallPlayerSyncGateComponent>();
}

void URecallPlayerSyncGateComponent::ServerPushEvent(uint32 Frame /*= 0*/)
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()))
	{
		return;
	}

	++ReplicatedEventCount;
#endif // WITH_SERVER_CODE
}

void URecallPlayerSyncGateComponent::ApplyEvent(uint32 Frame)
{
	++LocalAppliedEventCount;
	LastSyncedFrame = FMath::Max(LastSyncedFrame, Frame);

	if (!HasUnsyncedPlayerEvents())
	{
		OnPlayerSyncGateCaughtUp.Broadcast();
	}
}

void URecallPlayerSyncGateComponent::ApplyFlagEvent()
{
	++LocalAppliedEventCount;

	if (!HasUnsyncedPlayerEvents())
	{
		OnPlayerSyncGateCaughtUp.Broadcast();
	}
}

bool URecallPlayerSyncGateComponent::HasUnsyncedPlayerEvents() const
{
	return LocalAppliedEventCount < ReplicatedEventCount;
}

uint32 URecallPlayerSyncGateComponent::GetLastSyncedFrame() const
{
	return LastSyncedFrame;
}

void URecallPlayerSyncGateComponent::InitializeAppliedEventCountFromSnapshot(uint32 SnapshotEventCount, uint32 SnapshotFrame)
{
	// Add, don't Max/assign: LocalAppliedEventCount only ever advances via ApplyEvent/ApplyFlagEvent,
	// which fire from RPC/OnRep bodies for events actually received live - and a restoring client never
	// receives multicast RPCs for events that happened before it became a relevant connection (that's
	// exactly why the snapshot baseline exists). So by the time this runs, LocalAppliedEventCount holds
	// only events applied live SINCE connecting, a range entirely disjoint from SnapshotEventCount (events
	// already baked into the pre-connection snapshot). Using Max here would silently discard any events
	// this client already applied live while still mid-restore, permanently wedging the gate exactly like
	// the original bug this fixup exists to solve.
	LocalAppliedEventCount += SnapshotEventCount;
	LastSyncedFrame = FMath::Max(LastSyncedFrame, SnapshotFrame);

	if (!HasUnsyncedPlayerEvents())
	{
		OnPlayerSyncGateCaughtUp.Broadcast();
	}
}

void URecallPlayerSyncGateComponent::ResetGate()
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ReplicatedEventCount = 0;
	}
#endif // WITH_SERVER_CODE

	LocalAppliedEventCount = 0;
	LastSyncedFrame = 0;
}
