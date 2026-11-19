// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

// UE headers first, then Jolt (same order as JPRPhysicsSubsystem.cpp).
#include "CoreMinimal.h"
#include "Physics/JPRPhysicsBody.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/StateRecorderImpl.h>

namespace RecallTestPhysics
{
	namespace Layers
	{
		constexpr JPH::ObjectLayer NonMoving = 0;
		constexpr JPH::ObjectLayer Moving = 1;
	}

	namespace BroadPhaseLayers
	{
		constexpr JPH::BroadPhaseLayer NonMoving(0);
		constexpr JPH::BroadPhaseLayer Moving(1);
		constexpr JPH::uint Count = 2;
	}

	class FObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer InObject1, JPH::ObjectLayer InObject2) const override
		{
			switch (InObject1)
			{
			case Layers::NonMoving: return InObject2 == Layers::Moving;
			case Layers::Moving: return true;
			default: return false;
			}
		}
	};

	class FBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		virtual JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::Count; }

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer InLayer) const override
		{
			return InLayer == Layers::NonMoving ? BroadPhaseLayers::NonMoving : BroadPhaseLayers::Moving;
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer InLayer) const override
		{
			return InLayer == BroadPhaseLayers::NonMoving ? "NonMoving" : "Moving";
		}
#endif
	};

	class FObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer InLayer1, JPH::BroadPhaseLayer InLayer2) const override
		{
			switch (InLayer1)
			{
			case Layers::NonMoving: return InLayer2 == BroadPhaseLayers::Moving;
			case Layers::Moving: return true;
			default: return false;
			}
		}
	};

	/**
	 * A standalone Jolt world built the way UJPRPhysicsSubsystem::CreatePhysicsSystem builds it, without a UWorld.
	 * MaxBodies is configurable so tests can exhaust the BodyID space quickly.
	 */
	class FRecallTestPhysicsWorld
	{
	public:
		explicit FRecallTestPhysicsWorld(const JPH::uint InMaxBodies = 16384, const int32 InNumThreads = 2)
			: MaxBodies(InMaxBodies)
		{
			JPH::RegisterDefaultAllocator();
			// The factory is a process-wide singleton. Only tear it down if we created it.
			if (JPH::Factory::sInstance == nullptr)
			{
				JPH::Factory::sInstance = new JPH::Factory();
				JPH::RegisterTypes();
				bOwnsFactory = true;
			}

			TempAllocator = MakeShareable<JPH::TempAllocator>(new JPH::TempAllocatorImpl(64 * 1024 * 1024));
			JobSystem = MakeShared<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, InNumThreads);

			BroadPhaseLayerInterface = MakeShared<FBroadPhaseLayerInterface>();
			ObjectVsBroadPhaseLayerFilter = MakeShared<FObjectVsBroadPhaseLayerFilter>();
			ObjectLayerPairFilter = MakeShared<FObjectLayerPairFilter>();

			PhysicsSystem = MakeShared<JPH::PhysicsSystem>();
			PhysicsSystem->Init(MaxBodies, /*NumBodyMutexes*/ 0, /*MaxBodyPairs*/ MaxBodies * 4, /*MaxContactConstraints*/ MaxBodies * 2,
				*BroadPhaseLayerInterface, *ObjectVsBroadPhaseLayerFilter, *ObjectLayerPairFilter);
		}

		~FRecallTestPhysicsWorld()
		{
			PhysicsSystem.Reset();
			ObjectLayerPairFilter.Reset();
			ObjectVsBroadPhaseLayerFilter.Reset();
			BroadPhaseLayerInterface.Reset();
			JobSystem.Reset();
			TempAllocator.Reset();

			if (bOwnsFactory && JPH::Factory::sInstance != nullptr)
			{
				JPH::UnregisterTypes();
				delete JPH::Factory::sInstance;
				JPH::Factory::sInstance = nullptr;
			}
		}

		FRecallTestPhysicsWorld(const FRecallTestPhysicsWorld&) = delete;
		FRecallTestPhysicsWorld& operator=(const FRecallTestPhysicsWorld&) = delete;

		JPH::uint GetMaxBodies() const { return MaxBodies; }
		JPH::BodyInterface& Bodies() const { return PhysicsSystem->GetBodyInterface(); }

		void AddFloor()
		{
			JPH::BodyCreationSettings Settings(new JPH::BoxShape(JPH::Vec3(500.f, 1.f, 500.f)), JPH::RVec3(0, -1, 0),
				JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NonMoving);
			Bodies().CreateAndAddBody(Settings, JPH::EActivation::DontActivate);
		}

		void Step(const float DeltaTime, const int32 CollisionSteps = 1)
		{
			PhysicsSystem->Update(DeltaTime, CollisionSteps, TempAllocator.Get(), JobSystem.Get());
		}

		/** True if Jolt really owns a body with this exact ID (safe for IDs that were never allocated). */
		bool HasBody(const uint32 BodyIDAndSequence) const
		{
			return PhysicsSystem->GetBodyLockInterfaceNoLock().TryGetBody(JPH::BodyID(BodyIDAndSequence)) != nullptr;
		}

		JPH::uint GetNumBodies() const { return PhysicsSystem->GetNumBodies(); }

		JPH::uint GetNumConstraints() const { return static_cast<JPH::uint>(PhysicsSystem->GetConstraints().size()); }

		// Same as UJPRPhysicsSubsystem::CreateFixedConstraint.
		bool CreateFixedConstraint(const uint32 BodyID1, const uint32 BodyID2) const
		{
			JPH::FixedConstraintSettings Settings;
			Settings.mAutoDetectPoint = true;

			const JPH::BodyID ConstraintBodies[] = { JPH::BodyID(BodyID1), JPH::BodyID(BodyID2) };
			JPH::BodyLockMultiWrite Lock(PhysicsSystem->GetBodyLockInterface(), ConstraintBodies, UE_ARRAY_COUNT(ConstraintBodies));
			JPH::Body* Body1 = Lock.GetBody(0);
			JPH::Body* Body2 = Lock.GetBody(1);
			if (Body1 == nullptr || Body2 == nullptr)
			{
				return false;
			}

			JPH::TwoBodyConstraint* Constraint = Settings.Create(*Body1, *Body2);
			if (Constraint == nullptr)
			{
				return false;
			}
			PhysicsSystem->AddConstraint(Constraint);
			return true;
		}

		// Same as UJPRPhysicsSubsystem::RemoveFixedConstraints.
		void RemoveFixedConstraints(const uint32 BodyID1, const uint32 BodyID2) const
		{
			const JPH::BodyID ConstraintBodies[] = { JPH::BodyID(BodyID1), JPH::BodyID(BodyID2) };
			const JPH::Constraints Constraints = PhysicsSystem->GetConstraints();
			for (JPH::Constraint* Constraint : Constraints)
			{
				const JPH::TwoBodyConstraint* TwoBodyConstraint = static_cast<JPH::TwoBodyConstraint*>(Constraint);
				const JPH::Body* Body1 = TwoBodyConstraint->GetBody1();
				const JPH::Body* Body2 = TwoBodyConstraint->GetBody2();
				if (Body1 != nullptr && Body2 != nullptr
					&& (Body1->GetID() == ConstraintBodies[0] || Body1->GetID() == ConstraintBodies[1])
					&& (Body2->GetID() == ConstraintBodies[0] || Body2->GetID() == ConstraintBodies[1]))
				{
					PhysicsSystem->RemoveConstraint(Constraint);
				}
			}
		}

		// Same as UJPRPhysicsSubsystem::SavePhysicsState / RestorePhysicsState.
		TArray<uint8> SaveState(const TArray<TSharedPtr<FJPRPhysicsBody>>& InBodies) const
		{
			JPH::StateRecorderImpl State;
			PhysicsSystem->SaveState(State, JPH::EStateRecorderState::All, nullptr);
			for (const TSharedPtr<FJPRPhysicsBody>& Body : InBodies)
			{
				if (ensure(Body.IsValid()))
				{
					Body->SaveState(State);
				}
			}

			TArray<uint8> Result;
			const std::string& Data = State.GetData();
			Result.Append(reinterpret_cast<const uint8*>(Data.data()), Data.size());
			return Result;
		}

		bool RestoreState(const TArray<uint8>& StateData, const TArray<TSharedPtr<FJPRPhysicsBody>>& InBodies) const
		{
			JPH::StateRecorderImpl State;
			State.WriteBytes(StateData.GetData(), StateData.Num());
			State.Rewind();
			if (!PhysicsSystem->RestoreState(State, nullptr))
			{
				return false;
			}

			for (const TSharedPtr<FJPRPhysicsBody>& Body : InBodies)
			{
				if (ensure(Body.IsValid()))
				{
					Body->RestoreState(State);
				}
			}
			return true;
		}

		TSharedPtr<JPH::PhysicsSystem> PhysicsSystem;
		TSharedPtr<JPH::TempAllocator> TempAllocator;

	private:
		JPH::uint MaxBodies;
		bool bOwnsFactory = false;

		TSharedPtr<JPH::JobSystem> JobSystem;
		TSharedPtr<JPH::BroadPhaseLayerInterface> BroadPhaseLayerInterface;
		TSharedPtr<JPH::ObjectVsBroadPhaseLayerFilter> ObjectVsBroadPhaseLayerFilter;
		TSharedPtr<JPH::ObjectLayerPairFilter> ObjectLayerPairFilter;
	};

	/** A sphere body built on the real FJPRPhysicsBody so the production creation/release code is exercised. */
	class FRecallTestPhysicsBody final : public FJPRPhysicsBody
	{
	public:
		/** Returns true if Jolt accepted the body. */
		bool CreateSphere(const FRecallTestPhysicsWorld& World, const uint32 BodyID, const JPH::RVec3& Position, const float Radius = 0.5f)
		{
			SetPhysicsSystem(World.PhysicsSystem);
			SetTempAllocator(World.TempAllocator);

			JPH::BodyCreationSettings Settings(new JPH::SphereShape(Radius), Position,
				JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::Moving);
			return CreateAndSetBody(Settings, BodyID) != nullptr;
		}
	};
}
