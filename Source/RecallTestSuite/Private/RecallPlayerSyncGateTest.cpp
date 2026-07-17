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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPlayerSyncGateLateJoinTest, "Recall.PlayerSyncGate.LateJoinSyncsFromReplicated",
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

	// Restore finishes: the snapshot already reflects those events as of the current frame.
	Gate->SyncAppliedEventCountFromReplicated(30);
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
