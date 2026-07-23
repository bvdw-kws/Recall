// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/GameStateComponent.h"

#include "RecallPlayerSyncGateComponent.generated.h"

/**
 * Tracks critical replicated player state (AddPlayer/RemovePlayer game events, join/leave flags)
 * that must be locally applied before the confirm frame is allowed to advance past the frame they
 * take effect at.
 *
 * The server increments ReplicatedEventCount every time it issues one of these events (via ordinary
 * property replication). Every recipient (including the server itself) increments
 * LocalAppliedEventCount when it actually applies the event locally (e.g. inside the corresponding
 * multicast RPC, or an OnRep callback). While the two counters disagree, this client knows it has
 * not yet applied everything the server has issued, regardless of which of the two signals
 * (replicated property vs. RPC/OnRep) happened to arrive first.
 */
UCLASS()
class RECALLONLINE_API URecallPlayerSyncGateComponent : public UGameStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Gets a reference to the singleton instance of this component. */
	static URecallPlayerSyncGateComponent& GetRef(const UObject* WorldContextObject);
	
	/** Server only: record that a new critical event was issued for Frame. */
	void ServerPushEvent(uint32 Frame = 0);

	/** Called by every recipient (server included) when a critical event has actually been applied locally. */
	void ApplyEvent(uint32 Frame);

	/**
	 * Same as ApplyEvent, but for events whose frame isn't known/replicated on this recipient (e.g. a
	 * plain replicated bool flag with no attached frame). Counts the event as applied without advancing
	 * GetLastSyncedFrame() - conservative, since other frame-bearing events still drive that forward.
	 */
	void ApplyFlagEvent();

	/** Whether this client has applied fewer events than the server has issued. */
	bool HasUnsyncedPlayerEvents() const;

	/** Highest frame for which every critical event up to and including it has been applied locally. */
	uint32 GetLastSyncedFrame() const;

	/** Server only: current issued event count, to be captured alongside a restore snapshot. */
	uint32 GetReplicatedEventCount() const { return ReplicatedEventCount; }

	/**
	 * Late-join/restore fixup: historical events aren't replayed to a client that joins after they
	 * fired (it catches up via snapshot restore instead). SnapshotEventCount must be the event count
	 * captured atomically alongside the snapshot itself (see FRecallRestoreClientInfo::SnapshotEventCount),
	 * NOT the live ReplicatedEventCount at restore-completion time - any events issued after the snapshot
	 * was taken are still delivered normally via ApplyEvent/ApplyFlagEvent while this client is restoring,
	 * and must not be silently marked applied here.
	 */
	void InitializeAppliedEventCountFromSnapshot(uint32 SnapshotEventCount, uint32 SnapshotFrame);

	/** Reset the gate when the simulation restarts. */
	void ResetGate();

	DECLARE_MULTICAST_DELEGATE(FOnPlayerSyncGateCaughtUp);
	FOnPlayerSyncGateCaughtUp OnPlayerSyncGateCaughtUp;

	//~ Begin UObject Interface.
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface.

protected:
	/** Server-authoritative count of critical player events issued so far. */
	UPROPERTY(Replicated)
	uint32 ReplicatedEventCount = 0;

	/** Count of critical player events this client has actually applied locally. */
	UPROPERTY(Transient)
	uint32 LocalAppliedEventCount = 0;

	/** Frame of the most recently applied event (events are applied in non-decreasing frame order). */
	UPROPERTY(Transient)
	uint32 LastSyncedFrame = 0;
};
