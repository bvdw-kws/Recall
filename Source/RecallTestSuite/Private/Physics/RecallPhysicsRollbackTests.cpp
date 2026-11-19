// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "RecallTestPhysicsSimulation.h"

namespace RecallTestSuite::PhysicsRollback
{
	using namespace RecallTestPhysics;

	/** Deterministic grid layout so bodies start apart from each other, above the floor. */
	FVector3f GridPosition(const int32 Index)
	{
		return FVector3f((Index % 25) * 1.5f, 2.f + (Index / 625) * 1.5f, ((Index / 25) % 25) * 1.5f);
	}

	FString Join(const TArray<FString>& Lines, const int32 Max = 5)
	{
		FString Result;
		for (int32 Index = 0; Index < Lines.Num() && Index < Max; ++Index)
		{
			Result += (Index > 0 ? TEXT("; ") : TEXT("")) + Lines[Index];
		}
		if (Lines.Num() > Max)
		{
			Result += FString::Printf(TEXT("; (+%d more)"), Lines.Num() - Max);
		}
		return Result;
	}

	struct FSavedFrame
	{
		int32 Tick = -1;
		FRecallPhysicsSnapshot Snapshot;
		TArray<float> State;
	};
}

// -------------------------------------------------------------------------------------------------------------
// Regression for the root cause of the rollback assert: FJPRPhysicsBody::CreateAndSetBody used to record a BodyID even when Jolt
// refused to create the body (BodyID index >= MaxBodies, or the slot is already taken). Releasing such a body later
// is what trips BodyManager::RemoveBodyInternal during a rollback.
// -------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsBodyCreationFailureTest, "Recall.Physics.Rollback.BodyCreationFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsBodyCreationFailureTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestPhysics;

	// Both refusals below are provoked on purpose and logged as errors by FJPRPhysicsBody::CreateAndSetBody.
	AddExpectedError(TEXT("Jolt refused to create a body"), EAutomationExpectedErrorFlags::Contains, 2);

	constexpr JPH::uint MaxBodies = 64;
	FRecallTestPhysicsWorld World(MaxBodies);

	// A body whose ID index is beyond MaxBodies: Jolt cannot create it.
	{
		FRecallTestPhysicsBody Body;
		const bool bCreated = Body.CreateSphere(World, /*BodyID*/ MaxBodies + 10, JPH::RVec3(0, 5, 0));
		TestFalse(TEXT("Jolt refuses a BodyID beyond MaxBodies"), bCreated);
		TestTrue(TEXT("A body Jolt refused to create must not claim a BodyID (it would be released later with an ID Jolt never allocated)"),
			Body.GetBodyID() == JPH::BodyID::cInvalidBodyID || World.HasBody(Body.GetBodyID()));
	}

	// A body whose ID is already used by a live body: Jolt cannot create it either.
	{
		FRecallTestPhysicsBody First;
		TestTrue(TEXT("First body with ID 5 is created"), First.CreateSphere(World, 5, JPH::RVec3(0, 5, 0)));

		FRecallTestPhysicsBody Second;
		const bool bCreated = Second.CreateSphere(World, 5, JPH::RVec3(2, 5, 0));
		TestFalse(TEXT("Jolt refuses a BodyID that is already taken"), bCreated);
		TestTrue(TEXT("A refused duplicate must not claim the BodyID of the live body (releasing it would destroy the live body)"),
			Second.GetBodyID() == JPH::BodyID::cInvalidBodyID);
	}

	return true;
}

// -------------------------------------------------------------------------------------------------------------
// Basic save / restore behaviours of the tracker.
// -------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsRollbackRoundTripTest, "Recall.Physics.Rollback.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsRollbackRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	for (int32 Index = 0; Index < 50; ++Index)
	{
		Sim.SpawnBody(GridPosition(Index));
	}
	for (int32 Tick = 0; Tick < 10; ++Tick)
	{
		Sim.Step();
	}

	const FRecallPhysicsSnapshot Snapshot = Sim.Save();
	const TArray<float> SavedState = Sim.CaptureState();

	for (int32 Tick = 0; Tick < 30; ++Tick)
	{
		Sim.Step();
	}
	TestTrue(TEXT("Simulation moved on after the save"), Sim.CaptureState() != SavedState);

	Sim.Restore(Snapshot);

	TestTrue(TEXT("State is identical after restore"), Sim.CaptureState() == SavedState);
	TestEqual(TEXT("Bookkeeping is consistent after restore"), Join(Sim.Validate()), FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsRollbackSpawnAfterSaveTest, "Recall.Physics.Rollback.SpawnAfterSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsRollbackSpawnAfterSaveTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Sim.SpawnBody(GridPosition(Index));
	}
	Sim.Step();

	const FRecallPhysicsSnapshot Snapshot = Sim.Save();
	const TArray<float> SavedState = Sim.CaptureState();
	const uint32 SavedSerial = Sim.Tracker.GetSerialNumberGenerator();

	for (int32 Index = 20; Index < 30; ++Index)
	{
		Sim.SpawnBody(GridPosition(Index));
	}
	Sim.Step();

	Sim.Restore(Snapshot);

	TestEqual(TEXT("Bodies spawned after the save are released"), Sim.Tracker.GetBodyRefMap().Num(), 20);
	TestEqual(TEXT("Serial number generator is rolled back"), Sim.Tracker.GetSerialNumberGenerator(), SavedSerial);
	TestTrue(TEXT("State is identical after restore"), Sim.CaptureState() == SavedState);
	TestEqual(TEXT("Bookkeeping is consistent after restore"), Join(Sim.Validate()), FString());

	// Re-spawning reuses the rolled back serial numbers, exactly like a replayed simulation does.
	const FRecallPhysicsBodyHandle Respawned = Sim.SpawnBody(GridPosition(20));
	TestEqual(TEXT("Next serial after restore"), Respawned.SerialNumber, SavedSerial + 1);
	TestEqual(TEXT("Bookkeeping is consistent after respawn"), Join(Sim.Validate()), FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsRollbackDespawnAfterSaveTest, "Recall.Physics.Rollback.DespawnAfterSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsRollbackDespawnAfterSaveTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Sim.SpawnBody(GridPosition(Index));
	}
	for (int32 Tick = 0; Tick < 5; ++Tick)
	{
		Sim.Step();
	}

	const FRecallPhysicsSnapshot Snapshot = Sim.Save();
	const TArray<float> SavedState = Sim.CaptureState();
	const TArray<FRecallPhysicsBodyHandle> SavedHandles = Sim.GetSortedHandles();

	for (int32 Index = 0; Index < 20; Index += 2)
	{
		Sim.ReleaseBody(SavedHandles[Index]);
	}
	Sim.Step();
	TestEqual(TEXT("Half of the bodies are gone"), Sim.Tracker.GetBodyRefMap().Num(), 10);

	Sim.Restore(Snapshot);

	TestTrue(TEXT("Same handles after restore"), Sim.GetSortedHandles() == SavedHandles);
	TestTrue(TEXT("State is identical after restore"), Sim.CaptureState() == SavedState);
	TestEqual(TEXT("Bookkeeping is consistent after restore"), Join(Sim.Validate()), FString());
	return true;
}

// -------------------------------------------------------------------------------------------------------------
// Soak: thousands of bodies, simulated and saved every tick with spawn/despawn churn, regularly rolled back a few
// frames. Handle serial numbers (== Jolt BodyIDs) used to only grow, which exhausted the BodyID space of the Jolt
// world. They now wrap (FRecallPhysicsBodyTracker::AllocateHandle). MaxBodies is deliberately small so the wrap
// happens many times within the test.
// -------------------------------------------------------------------------------------------------------------

namespace RecallTestSuite::PhysicsRollback
{
	struct FChurnParams
	{
		JPH::uint MaxBodies = 8192;
		int32 InitialBodies = 1500;
		int32 ChurnPerTick = 20;
		int32 RingSize = 32;			// Mirrors RECALL_ROLLBACK_FRAME_BUFFER.
		int32 RollbackInterval = 53;
		int32 MaxRollbackFrames = 8;	// Mirrors RollbackFrameCount.
		int32 TargetTick = 600;
	};

	/**
	 * Entities keep despawning and spawning, every tick is saved in a ring and the simulation is regularly rolled back.
	 * Stops at the first body Jolt refuses and checks state and bookkeeping after every rollback.
	 */
	bool RunChurnSoak(FAutomationTestBase& Test, const FChurnParams& Params)
	{
		FRecallTestPhysicsSimulation Sim(Params.MaxBodies);
		for (int32 Index = 0; Index < Params.InitialBodies; ++Index)
		{
			Sim.SpawnBody(GridPosition(Index));
		}

		TArray<FSavedFrame> Ring;
		Ring.SetNum(Params.RingSize);

		auto SaveFrame = [&](const int32 Tick)
		{
			FSavedFrame& Frame = Ring[Tick % Params.RingSize];
			Frame.Tick = Tick;
			Frame.Snapshot = Sim.Save();
			Frame.State = Sim.CaptureState();
		};
		SaveFrame(0);

		int32 Tick = 0;
		int32 NextRollbackTick = Params.RollbackInterval;
		int32 NumRollbacks = 0;
		int32 NumSpawned = Params.InitialBodies;

		while (Tick < Params.TargetTick)
		{
			++Tick;

			// Scripted churn, a pure function of the tick so a replay does the same thing.
			FRandomStream Stream(Tick);
			TArray<FRecallPhysicsBodyHandle> Live = Sim.GetSortedHandles();
			for (int32 Index = 0; Index < Params.ChurnPerTick && !Live.IsEmpty(); ++Index)
			{
				const int32 Victim = Stream.RandRange(0, Live.Num() - 1);
				Sim.ReleaseBody(Live[Victim]);
				Live.RemoveAtSwap(Victim);
			}
			for (int32 Index = 0; Index < Params.ChurnPerTick; ++Index)
			{
				Sim.SpawnBody(FVector3f(Stream.FRandRange(0.f, 37.f), Stream.FRandRange(2.f, 25.f), Stream.FRandRange(0.f, 37.f)));
				++NumSpawned;
			}

			if (Sim.NumFailedSpawns > 0)
			{
				Test.AddError(FString::Printf(
					TEXT("Jolt refused to create body with serial %u (MaxBodies=%u) at tick %d after %d rollbacks, %d live bodies, %d bodies spawned so far."),
					Sim.FirstFailedSerial, Params.MaxBodies, Tick, NumRollbacks, Sim.Tracker.GetBodyRefMap().Num(), NumSpawned));
				return false;
			}

			Sim.Step();
			SaveFrame(Tick);

			if (Tick == NextRollbackTick)
			{
				NextRollbackTick += Params.RollbackInterval;

				const int32 Back = 1 + (Tick / Params.RollbackInterval) % Params.MaxRollbackFrames;
				const int32 RestoreTick = Tick - Back;
				const FSavedFrame& Frame = Ring[RestoreTick % Params.RingSize];
				if (!Test.TestEqual(TEXT("Ring frame holds the requested tick"), Frame.Tick, RestoreTick))
				{
					return false;
				}

				Sim.Restore(Frame.Snapshot);
				++NumRollbacks;
				Tick = RestoreTick;

				if (!Test.TestTrue(*FString::Printf(TEXT("State identical after rolling back to tick %d"), RestoreTick), Sim.CaptureState() == Frame.State))
				{
					return false;
				}

				const TArray<FString> Errors = Sim.Validate();
				if (!Test.TestTrue(*FString::Printf(TEXT("Bookkeeping consistent after rolling back to tick %d: %s"), RestoreTick, *Join(Errors)), Errors.IsEmpty()))
				{
					return false;
				}
			}
		}

		Test.AddInfo(FString::Printf(TEXT("Churn finished: %d ticks, %d rollbacks, %d live bodies, %d spawned in total, MaxBodies %u, last serial %u"),
			Params.TargetTick, NumRollbacks, Sim.Tracker.GetBodyRefMap().Num(), NumSpawned, Params.MaxBodies, Sim.Tracker.GetSerialNumberGenerator()));
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsRollbackSoakTest, "Recall.Physics.Rollback.Soak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsRollbackSoakTest::RunTest(const FString& Parameters)
{
	return RecallTestSuite::PhysicsRollback::RunChurnSoak(*this, RecallTestSuite::PhysicsRollback::FChurnParams{});
}

// -------------------------------------------------------------------------------------------------------------
// Game-like scale: ~10k bodies alive (60% of the BodyID space) with a high spawn/despawn rate. The total number of
// bodies ever created is several times MaxBodies, so serials have to wrap while most of the space is occupied.
// -------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsRollbackScaleTest, "Recall.Physics.Rollback.Scale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsRollbackScaleTest::RunTest(const FString& Parameters)
{
	RecallTestSuite::PhysicsRollback::FChurnParams Params;
	Params.MaxBodies = 16384;
	Params.InitialBodies = 10000;
	Params.ChurnPerTick = 150;
	Params.RollbackInterval = 37;
	Params.TargetTick = 400;
	return RecallTestSuite::PhysicsRollback::RunChurnSoak(*this, Params);
}

// -------------------------------------------------------------------------------------------------------------
// Constraints. Jolt keeps its own constraint objects, they have to follow the tracker's references on every
// release and restore, otherwise they outlive their bodies or survive a rollback that should have removed them.
// -------------------------------------------------------------------------------------------------------------

namespace RecallTestSuite::PhysicsRollback
{
	/** Spawns a few bodies and steps once so the saved state is not the initial one. */
	TArray<FRecallPhysicsBodyHandle> SpawnConstraintBodies(FRecallTestPhysicsSimulation& Sim, const int32 Count = 4)
	{
		TArray<FRecallPhysicsBodyHandle> Handles;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Handles.Add(Sim.SpawnBody(GridPosition(Index)));
		}
		Sim.Step();
		return Handles;
	}

	bool CheckConsistent(FAutomationTestBase& Test, const FRecallTestPhysicsSimulation& Sim, const TCHAR* When)
	{
		const TArray<FString> Errors = Sim.Validate();
		return Test.TestTrue(*FString::Printf(TEXT("Bookkeeping consistent %s: %s"), When, *Join(Errors)), Errors.IsEmpty());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsConstraintStaleOnRestoreTest, "Recall.Physics.Rollback.Constraints.StaleOnRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsConstraintStaleOnRestoreTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	const TArray<FRecallPhysicsBodyHandle> Handles = SpawnConstraintBodies(Sim);

	const FRecallPhysicsSnapshot Snapshot = Sim.Save();

	// A constraint created after the save must be gone after the rollback, in Jolt too.
	TestTrue(TEXT("Constraint created"), Sim.AddConstraint(Handles[0], Handles[1]));
	Sim.Step();
	TestEqual(TEXT("Jolt has the new constraint"), static_cast<int32>(Sim.World.GetNumConstraints()), 1);

	Sim.Restore(Snapshot);

	TestEqual(TEXT("Tracker has no constraint after restore"), Sim.Tracker.GetConstrainRefs().Num(), 0);
	TestEqual(TEXT("Jolt has no constraint after restore"), static_cast<int32>(Sim.World.GetNumConstraints()), 0);
	CheckConsistent(*this, Sim, TEXT("after restoring over a stale constraint"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsConstraintRecreatedOnRestoreTest, "Recall.Physics.Rollback.Constraints.RecreatedOnRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsConstraintRecreatedOnRestoreTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	const TArray<FRecallPhysicsBodyHandle> Handles = SpawnConstraintBodies(Sim);

	TestTrue(TEXT("Constraint created"), Sim.AddConstraint(Handles[0], Handles[1]));
	Sim.Step();
	const FRecallPhysicsSnapshot Snapshot = Sim.Save();
	const TArray<float> SavedState = Sim.CaptureState();

	// The constraint is removed after the save, the rollback must bring it back.
	Sim.RemoveAllConstraints(Handles[0], Handles[1]);
	Sim.Step();
	TestEqual(TEXT("Jolt has no constraint before restore"), static_cast<int32>(Sim.World.GetNumConstraints()), 0);

	Sim.Restore(Snapshot);

	TestEqual(TEXT("Tracker has the constraint after restore"), Sim.Tracker.GetConstrainRefs().Num(), 1);
	TestEqual(TEXT("Jolt has the constraint after restore"), static_cast<int32>(Sim.World.GetNumConstraints()), 1);
	TestTrue(TEXT("State identical after restore"), Sim.CaptureState() == SavedState);
	CheckConsistent(*this, Sim, TEXT("after recreating a constraint"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsConstraintReleaseBodyTest, "Recall.Physics.Rollback.Constraints.ReleaseBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsConstraintReleaseBodyTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	const TArray<FRecallPhysicsBodyHandle> Handles = SpawnConstraintBodies(Sim);

	TestTrue(TEXT("Constraint created"), Sim.AddConstraint(Handles[0], Handles[1]));

	// Releasing a constrained body must take its constraint with it, Jolt keeps raw pointers to constrained bodies.
	Sim.ReleaseBody(Handles[0]);

	TestEqual(TEXT("Tracker dropped the constraint"), Sim.Tracker.GetConstrainRefs().Num(), 0);
	TestEqual(TEXT("Jolt dropped the constraint"), static_cast<int32>(Sim.World.GetNumConstraints()), 0);
	for (int32 Step = 0; Step < 10; ++Step)
	{
		Sim.Step();
	}
	CheckConsistent(*this, Sim, TEXT("after releasing a constrained body"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsConstraintReleasedBodyRestoredTest, "Recall.Physics.Rollback.Constraints.ReleasedBodyRestored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsConstraintReleasedBodyRestoredTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	const TArray<FRecallPhysicsBodyHandle> Handles = SpawnConstraintBodies(Sim);

	TestTrue(TEXT("Constraint created"), Sim.AddConstraint(Handles[0], Handles[1]));
	Sim.Step();
	const FRecallPhysicsSnapshot Snapshot = Sim.Save();
	const TArray<float> SavedState = Sim.CaptureState();

	// One side of the constraint is despawned after the save. The rollback recreates the body and the constraint.
	Sim.ReleaseBody(Handles[0]);
	Sim.Step();

	Sim.Restore(Snapshot);

	TestEqual(TEXT("Tracker has the constraint after restore"), Sim.Tracker.GetConstrainRefs().Num(), 1);
	TestEqual(TEXT("Jolt has exactly one constraint after restore"), static_cast<int32>(Sim.World.GetNumConstraints()), 1);
	TestTrue(TEXT("State identical after restore"), Sim.CaptureState() == SavedState);
	CheckConsistent(*this, Sim, TEXT("after restoring a released constrained body"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsConstraintSpawnedAfterSaveTest, "Recall.Physics.Rollback.Constraints.SpawnedAfterSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsConstraintSpawnedAfterSaveTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	SpawnConstraintBodies(Sim);
	const FRecallPhysicsSnapshot Snapshot = Sim.Save();

	// Both bodies and their constraint appear after the save: the rollback releases the bodies and must drop the constraint first.
	const FRecallPhysicsBodyHandle Extra1 = Sim.SpawnBody(GridPosition(10));
	const FRecallPhysicsBodyHandle Extra2 = Sim.SpawnBody(GridPosition(11));
	TestTrue(TEXT("Constraint created"), Sim.AddConstraint(Extra1, Extra2));
	Sim.Step();

	Sim.Restore(Snapshot);

	TestEqual(TEXT("Tracker has no constraint after restore"), Sim.Tracker.GetConstrainRefs().Num(), 0);
	TestEqual(TEXT("Jolt has no constraint after restore"), static_cast<int32>(Sim.World.GetNumConstraints()), 0);
	for (int32 Step = 0; Step < 10; ++Step)
	{
		Sim.Step();
	}
	CheckConsistent(*this, Sim, TEXT("after restoring over bodies spawned with a constraint"));
	return true;
}

// -------------------------------------------------------------------------------------------------------------
// Teardown: URecallPhysicsSubsystem::ReleasePhysicsObjects releases every body while iterating BodyRefMap and passes
// It.Key() straight to ReleaseBody. The key lives inside the map element that ReleaseBody removes, so ReleaseBody must
// not read its handle parameter after the removal.
// -------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallPhysicsReleaseAllWhileIteratingTest, "Recall.Physics.Rollback.ReleaseAllWhileIterating",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallPhysicsReleaseAllWhileIteratingTest::RunTest(const FString& Parameters)
{
	using namespace RecallTestSuite::PhysicsRollback;

	FRecallTestPhysicsSimulation Sim;
	const TArray<FRecallPhysicsBodyHandle> Handles = SpawnConstraintBodies(Sim, 200);
	TestTrue(TEXT("Constraint created"), Sim.AddConstraint(Handles[0], Handles[1]));

	// Same loops as ReleasePhysicsObjects.
	const TSet<FRecallPhysicsConstrainHandle> Constrains = Sim.Tracker.GetConstrainRefs();
	for (const FRecallPhysicsConstrainHandle& Constrain : Constrains)
	{
		Sim.RemoveAllConstraints(Constrain.Body1, Constrain.Body2);
	}
	for (auto It = Sim.Tracker.GetBodyRefMap().CreateConstIterator(); It; ++It)
	{
		Sim.Tracker.ReleaseBody(Sim, It.Key());
	}

	TestEqual(TEXT("No body ref left"), Sim.Tracker.GetBodyRefMap().Num(), 0);
	TestEqual(TEXT("No handle left"), Sim.Tracker.GetBodyHandleMap().Num(), 0);
	TestEqual(TEXT("Jolt only owns the floor"), static_cast<int32>(Sim.World.GetNumBodies()), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
