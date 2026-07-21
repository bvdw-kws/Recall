// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/GameState/RecallPlayerSyncGateComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RecallTestSuite::PlayerSyncGate
{
	URecallPlayerSyncGateComponent* CreateGate(UWorld& World)
	{
		AActor* Owner = World.SpawnActor<AActor>();
		URecallPlayerSyncGateComponent* Gate = NewObject<URecallPlayerSyncGateComponent>(Owner);
		Gate->RegisterComponent();
		return Gate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPlayerSyncGateBasicTest, "Recall.PlayerSyncGate.BlocksUntilCaughtUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPlayerSyncGateBasicTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World created"), World);
	if (!World)
	{
		return false;
	}

	URecallPlayerSyncGateComponent* Gate = RecallTestSuite::PlayerSyncGate::CreateGate(*World);
	TestNotNull(TEXT("Gate created"), Gate);

	TestFalse(TEXT("Gate starts caught up"), Gate->HasUnsyncedPlayerEvents());
	TestEqual(TEXT("Gate starts with no synced frame"), Gate->GetLastSyncedFrame(), 0u);

	// Server issues an AddPlayer event for frame 100.
	Gate->ServerPushEvent(100);
	TestTrue(TEXT("Gate is behind after server pushes an event it hasn't applied"), Gate->HasUnsyncedPlayerEvents());
	TestEqual(TEXT("LastSyncedFrame unchanged while behind"), Gate->GetLastSyncedFrame(), 0u);

	// This client's multicast RPC for that event arrives and is applied.
	Gate->ApplyEvent(100);
	TestFalse(TEXT("Gate catches up once the event is applied"), Gate->HasUnsyncedPlayerEvents());
	TestEqual(TEXT("LastSyncedFrame advances to the applied event's frame"), Gate->GetLastSyncedFrame(), 100u);

	// A join/leave flag event with no attached frame still counts, but doesn't move LastSyncedFrame.
	Gate->ServerPushEvent(150);
	Gate->ApplyFlagEvent();
	TestFalse(TEXT("Flag event catches the gate up too"), Gate->HasUnsyncedPlayerEvents());
	TestEqual(TEXT("LastSyncedFrame is unaffected by a frame-less flag event"), Gate->GetLastSyncedFrame(), 100u);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPlayerSyncGateLateJoinTest, "Recall.PlayerSyncGate.LateJoinSyncsFromSnapshotBaseline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPlayerSyncGateLateJoinTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World created"), World);
	if (!World)
	{
		return false;
	}

	URecallPlayerSyncGateComponent* Gate = RecallTestSuite::PlayerSyncGate::CreateGate(*World);

	// Simulate a bunch of historical events issued by the server before this client connected.
	Gate->ServerPushEvent(10);
	Gate->ServerPushEvent(20);
	Gate->ServerPushEvent(30);
	TestTrue(TEXT("Gate is behind on historical events"), Gate->HasUnsyncedPlayerEvents());

	// A snapshot is captured atomically at this point: it reflects exactly the 3 events issued so
	// far, at count 3. The server sends this count alongside the snapshot bytes.
	const uint32 SnapshotEventCount = Gate->GetReplicatedEventCount();
	TestEqual(TEXT("Snapshot captures the 3 historical events"), SnapshotEventCount, 3u);

	// Restore finishes: install the baseline captured with the snapshot (not the live replicated count).
	Gate->InitializeAppliedEventCountFromSnapshot(SnapshotEventCount, 30);
	TestFalse(TEXT("Gate is caught up after the late-join fixup"), Gate->HasUnsyncedPlayerEvents());
	TestEqual(TEXT("LastSyncedFrame reflects the restore frame"), Gate->GetLastSyncedFrame(), 30u);

	// A genuinely new event after the client is live is still tracked normally.
	Gate->ServerPushEvent(40);
	TestTrue(TEXT("Gate blocks on a new event issued after restore"), Gate->HasUnsyncedPlayerEvents());
	Gate->ApplyEvent(40);
	TestFalse(TEXT("Gate catches up on the new event"), Gate->HasUnsyncedPlayerEvents());

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPlayerSyncGateSnapshotRaceTest, "Recall.PlayerSyncGate.SnapshotBaselineIgnoresInFlightEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPlayerSyncGateSnapshotRaceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World created"), World);
	if (!World)
	{
		return false;
	}

	URecallPlayerSyncGateComponent* Gate = RecallTestSuite::PlayerSyncGate::CreateGate(*World);

	// 3 historical events, snapshot captured here (count == 3, frame 30).
	Gate->ServerPushEvent(10);
	Gate->ServerPushEvent(20);
	Gate->ServerPushEvent(30);
	const uint32 SnapshotEventCount = Gate->GetReplicatedEventCount();
	const uint32 SnapshotFrame = 30;

	// While the snapshot is still in transit to the joining client, the server issues one more
	// event (e.g. another player joins/leaves) and multicasts it. This client (standing in as the
	// eventual restoring client's view of the world) receives and applies it before restore
	// finishes, exactly like it would for any other live client. It cannot be caught up yet though:
	// only 1 of the 4 events issued so far (10/20/30/40) has actually been applied locally - events
	// 10/20/30 were only ServerPushEvent'd above (simulating a client not yet connected when they
	// fired), never delivered via ApplyEvent. That gap is exactly what the snapshot baseline exists
	// to close.
	Gate->ServerPushEvent(40);
	Gate->ApplyEvent(40);
	TestTrue(TEXT("Still behind: events before the snapshot haven't been accounted for yet"),
		Gate->HasUnsyncedPlayerEvents());

	// Now restore "finishes" and installs the baseline captured earlier alongside the snapshot.
	// SnapshotEventCount (3) and LocalAppliedEventCount (1, from the live-applied event 40) describe
	// disjoint event ranges and must be added together, not maxed: 1 + 3 = 4, matching
	// ReplicatedEventCount exactly. A Max-based merge would incorrectly floor this at 3, permanently
	// wedging the gate one event behind even though event 40 was already fully applied.
	Gate->InitializeAppliedEventCountFromSnapshot(SnapshotEventCount, SnapshotFrame);
	TestFalse(TEXT("Gate is caught up: the live-applied event must be added to, not clobbered by, the baseline"),
		Gate->HasUnsyncedPlayerEvents());

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPlayerSyncGateResetTest, "Recall.PlayerSyncGate.ResetClearsState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPlayerSyncGateResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World created"), World);
	if (!World)
	{
		return false;
	}

	URecallPlayerSyncGateComponent* Gate = RecallTestSuite::PlayerSyncGate::CreateGate(*World);

	Gate->ServerPushEvent(5);
	Gate->ApplyEvent(5);
	TestEqual(TEXT("LastSyncedFrame set before reset"), Gate->GetLastSyncedFrame(), 5u);

	Gate->ResetGate();
	TestFalse(TEXT("Gate is caught up after reset"), Gate->HasUnsyncedPlayerEvents());
	TestEqual(TEXT("LastSyncedFrame cleared by reset"), Gate->GetLastSyncedFrame(), 0u);

	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
