// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "RecallTestPhysicsWorld.h"
#include "System/Physics/RecallPhysicsBodyTracker.h"
#include "System/Physics/RecallPhysicsSnapshot.h"

namespace RecallTestPhysics
{
	/**
	 * Does what URecallPhysicsSubsystem does around the tracker, on top of a standalone Jolt world:
	 * spawns/releases bodies, steps the simulation, takes snapshots and restores them.
	 * It mirrors URecallPhysicsSubsystem::CreateShape_Internal / ReleaseBody_Internal / Save / Restore.
	 */
	class FRecallTestPhysicsSimulation final : public IRecallPhysicsBodyOps
	{
	public:
		explicit FRecallTestPhysicsSimulation(const JPH::uint MaxBodies = 16384)
			: World(MaxBodies)
		{
			World.AddFloor();
			Tracker.SetMaxSerialNumber(MaxBodies - 1);
		}

		FRecallTestPhysicsWorld World;
		FRecallPhysicsBodyTracker Tracker;

		/** Number of times Jolt refused to create a body (the handle serial/BodyID was not accepted). */
		int32 NumFailedSpawns = 0;
		/** Serial of the first body Jolt refused to create, 0 if none. */
		uint32 FirstFailedSerial = 0;

		// ---- Subsystem-like API ---------------------------------------------------------------------

		/** Mirrors URecallPhysicsSubsystem::CreateShape. */
		FRecallPhysicsBodyHandle SpawnBody(const FVector3f& Position)
		{
			const FRecallPhysicsBodyHandle Handle = Tracker.AllocateHandle();
			if (!Handle.IsValid())
			{
				++NumFailedSpawns;
				return Handle;
			}
			CreateAndRegisterBody(Handle, Position);

			const FRecallPhysicsBodyRef* Ref = Tracker.FindBodyRef(Handle);
			if (Ref != nullptr && Ref->Body.IsValid() && World.HasBody(Ref->Body->GetBodyID()))
			{
				Ref->Body->Activate();
			}
			return Handle;
		}

		/** Mirrors URecallPhysicsSubsystem::ReleaseBody. */
		void ReleaseBody(const FRecallPhysicsBodyHandle& Handle)
		{
			Tracker.ReleaseBody(*this, Handle);
		}

		/** Mirrors URecallPhysicsSubsystem::ReleasePhysicsObjects. */
		void ReleaseAll()
		{
			Tracker.ReleaseAll(*this);
		}

		/** Mirrors URecallPhysicsSubsystem::CreateFixedConstrain. Returns false if it could not be created. */
		bool AddConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
		{
			const FRecallPhysicsBodyRef* Ref1 = Tracker.FindBodyRef(Handle1);
			const FRecallPhysicsBodyRef* Ref2 = Tracker.FindBodyRef(Handle2);
			if (Ref1 == nullptr || Ref2 == nullptr || !Ref1->Body.IsValid() || !Ref2->Body.IsValid()
				|| !World.CreateFixedConstraint(Ref1->Body->GetBodyID(), Ref2->Body->GetBodyID()))
			{
				return false;
			}
			Tracker.AddConstrainRef(Handle1, Handle2);
			return true;
		}

		/** Mirrors URecallPhysicsSubsystem::RemoveAllConstrains. */
		void RemoveAllConstraints(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
		{
			World.RemoveFixedConstraints(Handle1.SerialNumber, Handle2.SerialNumber);
			Tracker.RemoveConstrainRef(Handle1, Handle2);
		}

		void Step(const float DeltaTime = 1.f / 60.f)
		{
			World.Step(DeltaTime);
		}

		FRecallPhysicsSnapshot Save() const
		{
			FRecallPhysicsSnapshot Snapshot;
			// Const-cast free: the tracker only reads, ops are ours.
			FRecallTestPhysicsSimulation& MutableThis = const_cast<FRecallTestPhysicsSimulation&>(*this);
			Tracker.Save(MutableThis, Snapshot);
			return Snapshot;
		}

		void Restore(const FRecallPhysicsSnapshot& Snapshot)
		{
			Tracker.Restore(*this, Snapshot);
		}

		TArray<FRecallPhysicsBodyHandle> GetSortedHandles() const
		{
			return Tracker.GetHandlesSortedBySerialNumber();
		}

		/** Position + linear velocity of every tracked body ordered by serial number, as raw floats. */
		TArray<float> CaptureState() const
		{
			TArray<float> State;
			for (const FRecallPhysicsBodyHandle& Handle : GetSortedHandles())
			{
				const FRecallPhysicsBodyRef& Ref = Tracker.GetBodyRefMap().FindChecked(Handle);
				const JPH::BodyID Id(Ref.Body->GetBodyID());
				const JPH::RVec3 Position = World.Bodies().GetCenterOfMassPosition(Id);
				const JPH::Vec3 Velocity = World.Bodies().GetLinearVelocity(Id);
				State.Add(static_cast<float>(Handle.SerialNumber));
				State.Add(static_cast<float>(Position.GetX()));
				State.Add(static_cast<float>(Position.GetY()));
				State.Add(static_cast<float>(Position.GetZ()));
				State.Add(Velocity.GetX());
				State.Add(Velocity.GetY());
				State.Add(Velocity.GetZ());
			}
			return State;
		}

		/**
		 * Checks that the tracker bookkeeping and Jolt agree. Returns the list of problems (empty when consistent).
		 * Safe to call even when a body claims an ID Jolt never allocated.
		 */
		TArray<FString> Validate() const
		{
			TArray<FString> Errors;

			if (Tracker.GetBodyRefMap().Num() != Tracker.GetBodyHandleMap().Num())
			{
				Errors.Add(FString::Printf(TEXT("BodyRefMap has %d entries but BodyHandleMap has %d"),
					Tracker.GetBodyRefMap().Num(), Tracker.GetBodyHandleMap().Num()));
			}

			for (const TPair<FRecallPhysicsBodyHandle, FRecallPhysicsBodyRef>& Pair : Tracker.GetBodyRefMap())
			{
				const FRecallPhysicsBodyHandle& Handle = Pair.Key;
				const FRecallPhysicsBodyRef& Ref = Pair.Value;
				if (!Ref.Body.IsValid())
				{
					Errors.Add(FString::Printf(TEXT("Body<%u> is tracked without a body object"), Handle.SerialNumber));
					continue;
				}
				if (!World.HasBody(Ref.Body->GetBodyID()))
				{
					Errors.Add(FString::Printf(TEXT("Body<%u> is tracked (BodyID 0x%08x) but Jolt does not own that body"),
						Handle.SerialNumber, Ref.Body->GetBodyID()));
				}
			}

			// Jolt has the floor plus one body per tracked body.
			const int32 ExpectedJoltBodies = Tracker.GetBodyRefMap().Num() + 1;
			if (static_cast<int32>(World.GetNumBodies()) != ExpectedJoltBodies)
			{
				Errors.Add(FString::Printf(TEXT("Jolt owns %d bodies but %d are expected (tracked + floor)"),
					static_cast<int32>(World.GetNumBodies()), ExpectedJoltBodies));
			}

			// Every tracked constraint is a live Jolt constraint, and Jolt has no other.
			if (static_cast<int32>(World.GetNumConstraints()) != Tracker.GetConstrainRefs().Num())
			{
				Errors.Add(FString::Printf(TEXT("Jolt owns %d constraints but %d are tracked"),
					static_cast<int32>(World.GetNumConstraints()), Tracker.GetConstrainRefs().Num()));
			}
			return Errors;
		}

		// ---- IRecallPhysicsBodyOps ------------------------------------------------------------------

		virtual void CreateBody(const FRecallPhysicsBodyHandle& Handle, const FJPRPhysicsBodySnapshot& Snapshot) override
		{
			// The position is irrelevant: the Jolt state restore that follows overwrites it.
			CreateAndRegisterBody(Handle, FVector3f::ZeroVector);
		}

		virtual void CreateConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2) override
		{
			AddConstraint(Handle1, Handle2);
		}

		virtual void RemoveConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2) override
		{
			World.RemoveFixedConstraints(Handle1.SerialNumber, Handle2.SerialNumber);
		}

		virtual void SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, uint16 Layer) override {}
		virtual void ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle) override {}
		virtual void SetMotionType(const FRecallPhysicsBodyHandle& Handle, EJPRPhysicsMotionType MotionType) override {}
		virtual void ResetMotionType(const FRecallPhysicsBodyHandle& Handle) override {}

		virtual TArray<uint8> SaveState(const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies) override
		{
			return World.SaveState(Bodies);
		}

		virtual bool RestoreState(const TArray<uint8>& StateData, const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies) override
		{
			return World.RestoreState(StateData, Bodies);
		}

	private:
		/** Mirrors URecallPhysicsSubsystem::CreateShape_Internal: create the body, then register it whatever the outcome. */
		void CreateAndRegisterBody(const FRecallPhysicsBodyHandle& Handle, const FVector3f& Position)
		{
			TSharedPtr<FRecallTestPhysicsBody> Body = MakeShared<FRecallTestPhysicsBody>();
			if (!Body->CreateSphere(World, Handle.SerialNumber, JPH::RVec3(Position.X, Position.Y, Position.Z)))
			{
				if (NumFailedSpawns == 0)
				{
					FirstFailedSerial = Handle.SerialNumber;
				}
				++NumFailedSpawns;
			}

			FRecallPhysicsBodyRef Ref;
			Ref.Body = Body;
			Tracker.RegisterBody(Handle, MoveTemp(Ref));
		}
	};
}
