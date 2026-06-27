// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Physics/RecallPhysicsSubsystem.h"

#include "Async/ParallelFor.h"
#include "Physics/JPRPhysicsLayerDataAsset.h"
#include "Desync/RecallDesyncLog.h"
#include "Kismet/GameplayStatics.h"
#include "Landscape.h"
#include "RecallPhysicsSnapshot.h"
#include "Physics/Factory/RecallPhysicsObjectFactory.h"
#include "Physics/JPRPhysicsMath.h"
#include "Physics/RecallPhysicsObjects.h"
#include "Physics/RecallPhysicsShapeTypes.h"
#include "Settings/RecallSimulationSettings.h"
#include "Settings/JPRPhysicsSettings.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "Utility/Physics/RecallPhysicsUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"
#include "Utility/Slowmo/RecallSlowmoUtils.h"
#include "World/RecallWorldSettings.h"

DEFINE_LOG_CATEGORY(LogRecallPhysics);

#if WITH_JOLT_PHYSICS
// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/StateRecorderImpl.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
{
	UE_LOG(LogRecallPhysics, Error, TEXT("%s:%s: (%d) %s"), *FString(inExpression), *FString(inFile), inLine, *FString(inMessage));

	UE_DEBUG_BREAK();
	return true;
};

#endif // JPH_ENABLE_ASSERTS

struct FRecallContactEvent
{
	FRecallContactEvent(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold)
		: BodyID1(inBody1.GetID())
		, BodyID2(inBody2.GetID())
		, Vel1(inBody1.GetLinearVelocity())
		, Vel2(inBody2.GetLinearVelocity())
		, bIsStatic1(inBody1.IsStatic())
		, bIsStatic2(inBody2.IsStatic())
		, bIsSensor1(inBody1.IsSensor())
		, bIsSensor2(inBody2.IsSensor())
		, ImpactPoint1(inManifold.GetWorldSpaceContactPointOn1(0))
		, ImpactPoint2(inManifold.GetWorldSpaceContactPointOn2(0))
		, ImpactNormal1(inManifold.mWorldSpaceNormal)
		, ImpactNormal2(inManifold.SwapShapes().mWorldSpaceNormal)
		, SubShapeID1(inManifold.mSubShapeID1.GetValue())
		, SubShapeID2(inManifold.mSubShapeID2.GetValue())
	{
	}

	const BodyID BodyID1;
	const BodyID BodyID2;
	const Vec3 Vel1;
	const Vec3 Vel2;
	const bool bIsStatic1;
	const bool bIsStatic2;
	const bool bIsSensor1;
	const bool bIsSensor2;
	const Vec3 ImpactPoint1;
	const Vec3 ImpactPoint2;
	const Vec3 ImpactNormal1;
	const Vec3 ImpactNormal2;
	const uint32 SubShapeID1;
	const uint32 SubShapeID2;
};

/// User callbacks that allow determining which parts of the simulation should be saved by a StateRecorder
class FRecallStateRecorderFilter : public StateRecorderFilter
{
public:
	FRecallStateRecorderFilter(const URecallPhysicsSubsystem& inPhysicsSystem)
		: PhysicsSystem(inPhysicsSystem)
	{
	}

protected:
	///@}
	///@name Functions called during SaveState
	///@{

	/// If the state of a specific body should be saved
	virtual bool		ShouldSaveBody([[maybe_unused]] const Body& inBody) const override
	{
		// For static bodies, check if they should be restored based on entity system
		if (inBody.IsStatic() && !inBody.CanBeKinematicOrDynamic())
		{
			const uint32 BodyID = inBody.GetID().GetIndexAndSequenceNumber();
			return PhysicsSystem.ShouldRestorePhysicsBody(BodyID);
		}
		
		// Non-static bodies should always be saved
		return true;
	}

	/// If the state of a specific constraint should be saved
	virtual bool		ShouldSaveConstraint([[maybe_unused]] const Constraint& inConstraint) const override
	{
		return true;
	}

	/// If the state of a specific contact should be saved
	virtual bool		ShouldSaveContact([[maybe_unused]] const BodyID& inBody1, [[maybe_unused]] const BodyID& inBody2) const override
	{
		return true;
	}

	///@}
	///@name Functions called during RestoreState
	///@{

	/// If the state of a specific contact should be restored
	virtual bool		ShouldRestoreContact([[maybe_unused]] const BodyID& inBody1, [[maybe_unused]] const BodyID& inBody2) const override
	{
		return true;
	}

private:
	const URecallPhysicsSubsystem& PhysicsSystem;
};

// An example contact listener
class FRecallContactListener : public ContactListener
{
public:
	// See: ContactListener
	virtual ValidateResult	OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
	{
		UE_LOG(LogRecallPhysics, VeryVerbose, TEXT("Contact validate callback"));

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void			OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
		UE_LOG(LogRecallPhysics, VeryVerbose, TEXT("A contact was added"));

		FScopeLock Lock(&DataGuard);
		contact_events.Add(FRecallContactEvent(inBody1, inBody2, inManifold));
	}

	virtual void			OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
		UE_LOG(LogRecallPhysics, VeryVerbose, TEXT("A contact was persisted"));

		FScopeLock Lock(&DataGuard);
		contact_events.Add(FRecallContactEvent(inBody1, inBody2, inManifold));
	}

	virtual void			OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
	{
		UE_LOG(LogRecallPhysics, VeryVerbose, TEXT("A contact was removed"));
	}

	TArray<FRecallContactEvent> contact_events;

private:
	FCriticalSection DataGuard;
};

// An example activation listener
class FRecallBodyActivationListener : public BodyActivationListener
{
public:
	virtual void		OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		UE_LOG(LogRecallPhysics, Verbose, TEXT("A body got activated"));
	}

	virtual void		OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		UE_LOG(LogRecallPhysics, Verbose, TEXT("A body went to sleep"));
	}
};

struct FRecallPhysicsHit
{
	const Body*			mBody = nullptr;
	SubShapeID			mSubShapeID2;
	RVec3				mContactPosition;
	Vec3				mContactNormal;
	float				mPenetrationDepth;
};
#endif // WITH_JOLT_PHYSICS

void URecallPhysicsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallEntitySubsystem>();

	EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(GetWorld());
	
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	PhysicsLayer = SimulationSettings->DefaultLayer.LoadSynchronous();

	if (const UWorld* World = GetWorld())
	{
		if (const ARecallWorldSettings* WorldSettings = Cast<ARecallWorldSettings>(World->GetWorldSettings()))
		{
			if (const TObjectPtr<UJPRPhysicsLayerDataAsset>& OverridePhysicsLayer = WorldSettings->GetOverridePhysicsLayer())
			{
				PhysicsLayer = OverridePhysicsLayer;
			}
		}
	}

	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::OnActorsInitialized);

	CreatePhysicsSystem();
}

void URecallPhysicsSubsystem::Deinitialize()
{
	Super::Deinitialize();

	EntitySystem.Reset();
	PhysicsLayer = nullptr;

	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);

	ReleasePhysicsObjects();
	DeletePhysicsSystem();
}

void URecallPhysicsSubsystem::Start(const FRecallSimulationStartParams& Params)
{
	if (PhysicsLayer)
	{
		if (LandscapeActor.IsValid() && PhysicsLayer->ShouldUseLandscapeCollision())
		{
			Recall::Physics::Utils::CreateLandscapeShape(*this, *LandscapeActor, PhysicsLayer->GetLandscapeFriction());		
		}

		if (PhysicsLayer->ShouldUseWorldStaticMeshCollision())
		{
			Recall::Physics::Utils::CreateStaticMeshShapes(*this, PhysicsLayer->GetWorldStaticMeshFriction());		
		}
	}
}

void URecallPhysicsSubsystem::Reset()
{
	SerialNumberGenerator = 0;

	ReleasePhysicsObjects();

	DeletePhysicsSystem();
	CreatePhysicsSystem();
}

void URecallPhysicsSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_Save);

	{		
		FScopeLock Lock(&HitEventGuard);
		checkf(HitEvents.IsEmpty(), TEXT("Should be reset at the end of each frame"));
	}

	OutSnapshot.InitializeAs<FRecallPhysicsSnapshot>();

	FRecallPhysicsSnapshot& Snasphot = OutSnapshot.GetMutable<FRecallPhysicsSnapshot>();
	Snasphot.SerialNumberGenerator = SerialNumberGenerator;

	constexpr int32 PhysicsSaveTaskCount = 3;
	ParallelFor(PhysicsSaveTaskCount, [this, &Snasphot](int32 Index)
		{
			switch (Index)
			{
			case 0: // Save State
			{
#if WITH_JOLT_PHYSICS
				StateRecorderImpl State;
				SaveState(State);

				const std::string Data = State.GetData();
				const std::vector<char> Bytes(Data.begin(), Data.end());

				Snasphot.Data.SetNumUninitialized(Bytes.size());

				for (int32 ByteIndex = 0; ByteIndex < Bytes.size(); ByteIndex++)
				{
					Snasphot.Data[ByteIndex] = Bytes[ByteIndex];
				}
#endif // WITH_JOLT_PHYSICS
			}
			break;

			case 1: // Save Bodies
			{
				Snasphot.Bodies.Reserve(BodyRefMap.Num());

				for (const TPair<FRecallPhysicsBodyHandle, FRecallPhysicsBodyRef>& PhysicsBodyRef : BodyRefMap)
				{
					if (!PhysicsBodyRef.Value.bRestoreBody)
					{
						continue;
					}
					
					const FRecallPhysicsBodySnapshot BodySnapshot = TakeBodySnapshot(PhysicsBodyRef.Key);
					Snasphot.Bodies.Add(PhysicsBodyRef.Key, BodySnapshot);
				}
			}
			break;
				
			case 2: // Save Constrains
				Snasphot.Constrains = ConstrainRefs;
				break;
				
			default:
				unimplemented();
				break;
			}
		}
	);
}

void URecallPhysicsSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_Restore);

	const FRecallPhysicsSnapshot* InData = InSnapshot.GetPtr<FRecallPhysicsSnapshot>();
	if (InData == nullptr)
	{
		return;
	}

	SerialNumberGenerator = InData->SerialNumberGenerator;

	TSet<FRecallPhysicsConstrainHandle> NewConstrains = InData->Constrains;
	const TSet<FRecallPhysicsConstrainHandle> OldConstrains = ConstrainRefs;
	
	TSet<FRecallPhysicsBodyHandle> NewHandles;
	InData->Bodies.GetKeys(NewHandles);

	TArray<FRecallPhysicsBodyHandle> OldHandles;
	BodyRefMap.GenerateKeyArray(OldHandles);

	TSet<FRecallPhysicsBodyHandle> ReleasedHandles;
	
	for (const FRecallPhysicsBodyHandle& OldHandle : OldHandles)
	{
		// No need to take care of static bodies since they are not saved.
		const FRecallPhysicsBodyRef& OldBodyRef = BodyRefMap[OldHandle];
		if (!OldBodyRef.bRestoreBody)
		{
			checkf(!NewHandles.Contains(OldHandle),
				TEXT("%hs Should not save bodies that must not be restored"), __FUNCTION__);
			continue;
		}
		
		// Keep existing bodies so they can be restored.
		if (NewHandles.Contains(OldHandle))
		{
			const FRecallPhysicsBodyRef& NewBodySnapshot = BodyRefMap[OldHandle];
			if (NewBodySnapshot.Shape == OldBodyRef.Shape && NewBodySnapshot.Params == OldBodyRef.Params)
			{
				NewHandles.Remove(OldHandle);
				continue;
			}			
		}

		// Destroy non-existing bodies
		ReleaseBody_Internal(OldHandle);
		ReleasedHandles.Add(OldHandle);
	}

	// Release deprecated constrains
	for (const FRecallPhysicsConstrainHandle& ConstrainRef : OldConstrains)
	{
		if (NewConstrains.Contains(ConstrainRef))
		{
			if (!ReleasedHandles.Contains(ConstrainRef.Body1) && !ReleasedHandles.Contains(ConstrainRef.Body2))
			{
				NewConstrains.Remove(ConstrainRef);
				continue;
			}
		}
		
		RemoveConstrainRef_Internal(ConstrainRef.Body1, ConstrainRef.Body2);
	}

	// Create non-existing bodies
	for (FRecallPhysicsBodyHandle& NewHandle : NewHandles)
	{
		check(NewHandle.IsValid());
		const FRecallPhysicsBodySnapshot& BodySnapshot = InData->Bodies[NewHandle];
		const FRecallPhysicsShape& Shape = BodySnapshot.Shape.Get<FRecallPhysicsShape>();

		CreateShape_Internal(BodySnapshot.Entity, BodySnapshot.Shape, Shape.FactoryClass, BodySnapshot.Params, NewHandle);
	}

	// Re-apply body settings
	for (const TPair<FRecallPhysicsBodyHandle, FRecallPhysicsBodySnapshot>& RestorePair : InData->Bodies)
	{
		const FRecallPhysicsBodyHandle& Handle = RestorePair.Key;

		const FRecallPhysicsBodyRef& BodyRef = BodyRefMap.FindChecked(Handle);
		const TSharedPtr<FRecallPhysicsBody>& Body = BodyRef.Body;
		check(Body.IsValid());
		
		const FRecallPhysicsBodySnapshot& BodySnapshot = RestorePair.Value;

		if (BodySnapshot.bEnabled)
		{
			Body->Activate();
		}
		else
		{
			Body->Desactivate();
		}

		if (BodySnapshot.bUseLayerOverride)
		{
			SetLayerOverride(Handle, BodySnapshot.LayerOverride);
		}
		else if (BodyRef.LayerOverride.IsValid())
		{
			ClearLayerOverride(Handle);
		}

		if (BodySnapshot.bUseMotionTypeOverride)
		{
			SetMotionType(Handle, BodySnapshot.MotionTypeOverride);
		}
		else if (BodyRef.MotionTypeOverride.IsValid())
		{
			ResetMotionType(Handle);
		}
	}

	// Create new constrains
	for (const FRecallPhysicsConstrainHandle& ConstrainSnapshot : NewConstrains)
	{
		CreateFixedConstrain(ConstrainSnapshot.Body1, ConstrainSnapshot.Body2);
	}

#if WITH_JOLT_PHYSICS
	StateRecorderImpl State;
	State.WriteBytes(InData->Data.GetData(), InData->Data.Num());
	State.Rewind();

	RestoreState(State);
#endif // WITH_JOLT_PHYSICS
}

FRecallPhysicsBodySnapshot URecallPhysicsSubsystem::TakeBodySnapshot(const FRecallPhysicsBodyHandle& Handle) const
{
	const FRecallPhysicsBodyRef& PhysicsBodyRef = BodyRefMap.FindChecked(Handle);
	
	FRecallPhysicsBodySnapshot BodySnapshot;
	BodySnapshot.Entity = PhysicsBodyRef.Entity;
	BodySnapshot.Shape = PhysicsBodyRef.Shape;
	BodySnapshot.Params = PhysicsBodyRef.Params;

	if (ensure(PhysicsBodyRef.Body.IsValid()))
	{
		BodySnapshot.bEnabled = PhysicsBodyRef.Body->IsEnabled();
	}

	if (PhysicsBodyRef.LayerOverride.IsValid())
	{
		BodySnapshot.bUseLayerOverride = true;
		BodySnapshot.LayerOverride = *PhysicsBodyRef.LayerOverride.Get();
	}

	if (PhysicsBodyRef.MotionTypeOverride.IsValid())
	{
		BodySnapshot.bUseMotionTypeOverride = true;
		BodySnapshot.MotionTypeOverride = *PhysicsBodyRef.MotionTypeOverride.Get();
	}

	return BodySnapshot;
}

void URecallPhysicsSubsystem::ReleasePhysicsObjects()
{
	for (auto It = ConstrainRefs.CreateConstIterator(); It; ++It)
	{
		RemoveAllConstrains(It->Body1, It->Body2);
	}
	check(ConstrainRefs.IsEmpty());
	
	for (auto It = BodyRefMap.CreateConstIterator(); It; ++It)
	{
		ReleaseBody_Internal(It.Key(), true);
	}
	check(BodyHandleMap.IsEmpty());
	check(BodyRefMap.IsEmpty());
}

void URecallPhysicsSubsystem::TickPhysics(float DeltaTime)
{
#if WITH_JOLT_PHYSICS
	if (PhysicsSystem.IsValid())
	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_TickPhysics);
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallPhysicsSubsystem::TickPhysics Update")));

		const UJPRPhysicsSettings* PhysicsSettings = GetDefault<UJPRPhysicsSettings>();
		const float TimeDilatation = Recall::Slowmo::Utils::GetTimeDilatation(this);
		StepPhysics(DeltaTime, PhysicsSettings->CollisionSteps, TimeDilatation);
	}
#endif // WITH_JOLT_PHYSICS
}

bool URecallPhysicsSubsystem::ShouldGenerateHitEvent(const uint32 BodyID) const
{
	// Some custom colliders may not be registered in this map but still generate hit events
	if (const FRecallPhysicsBodyHandle* BodyHandlePtr = BodyHandleMap.Find(BodyID))
	{
		const TWeakPtr<const FRecallPhysicsBody> BodyPtr = GetBody(*BodyHandlePtr);
		if (BodyPtr.IsValid() && BodyPtr.Pin()->DoesTriggerHitEvents() == false)
		{
			return false;
		}
	}

	return true;
}

bool URecallPhysicsSubsystem::ShouldRestorePhysicsBody(const uint32 BodyID) const
{
	if (const FRecallPhysicsBodyHandle* BodyHandlePtr = BodyHandleMap.Find(BodyID))
	{
		if (const FRecallPhysicsBodyRef* BodyRefPtr = BodyRefMap.Find(*BodyHandlePtr))
		{
			// Only restore bodies that are marked for restoration
			// Static entities without mutable flag should not be restored
			return BodyRefPtr->bRestoreBody;
		}
	}

	// Default to not restoring if we can't find the body reference
	return false;
}

void URecallPhysicsSubsystem::GenerateHitEvents(const TSet<FRecallPhysicsBodyHandle>& GeneratesHitEventsBodyHandles)
{
	checkf(HitEvents.IsEmpty(), TEXT("Previous frame hit events should have been emptied during the last frame."));

	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_GenerateHitEvents);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsSubsystem::GenerateHitEvents"));

#if WITH_JOLT_PHYSICS
	auto TryPushHitEvent = [&](uint32 HitID, const BodyID& BodyID1, const BodyID& BodyID2, const Vec3& Vel1, const Vec3& Vel2,
		const Vec3& ImpactPoint, const Vec3& ImpactNormal, bool bHitStatic, bool bHitSensor)
	{
		if (!ensure(!BodyID1.IsInvalid() && !BodyID2.IsInvalid()))
		{
			return;
		}

		const uint32 Body1ID = BodyID1.GetIndexAndSequenceNumber();
		const FRecallPhysicsBodyHandle* Body1HandlePtr = BodyHandleMap.Find(Body1ID);

		if (Body1HandlePtr == nullptr || !GeneratesHitEventsBodyHandles.Contains(*Body1HandlePtr))
		{
			return;
		}

		const uint32 Body2ID = BodyID2.GetIndexAndSequenceNumber();
		if (ShouldGenerateHitEvent(Body2ID) == false)
		{
			return;
		}

		FRecallPhysicsHitEvent HitEvent;
		HitEvent.HitID = HitID;

		if (const FRecallPhysicsBodyHandle* Body2HandlePtr = BodyHandleMap.Find(Body2ID))
		{
			HitEvent.ColliderBodyHandle = *Body2HandlePtr;

			if (const FRecallPhysicsBodyRef* Body2Ref = BodyRefMap.Find(*Body2HandlePtr))
			{
				// Prevent the entity from detecting hit with itself
				const FRecallPhysicsBodyRef* Body1Ref = BodyRefMap.Find(*Body1HandlePtr); 
				if (Body1Ref != nullptr && Body1Ref->Entity == Body2Ref->Entity)
				{
					return;
				}
				
				HitEvent.ColliderEntity = Body2Ref->Entity;
			}
		}

		HitEvent.ImpactWorldLocation = JoltPhysicsToUnreal(FVector(ImpactPoint.GetX(), ImpactPoint.GetY(), ImpactPoint.GetZ()));
		HitEvent.ImpactWorldNormal = JoltPhysicsToUnrealDirection(FVector(ImpactNormal.GetX(), ImpactNormal.GetY(), ImpactNormal.GetZ()));
		HitEvent.bHitStatic = bHitStatic;
		HitEvent.bHitSensor = bHitSensor;

		HitEvent.WorldVelocityAtImpact = JoltPhysicsToUnreal(FVector(Vel1.GetX(), Vel1.GetY(), Vel1.GetZ()));

		{
			FScopeLock Lock(&HitEventGuard);
			TArray<FRecallPhysicsHitEvent>& Hits = HitEvents.FindOrAdd(*Body1HandlePtr);
			int32 InsertIndex = 0;
			for (int32 HitIndex = Hits.Num() - 1; HitIndex >= 0; HitIndex--)
			{
				if (Hits[HitIndex].HitID < HitEvent.HitID)
				{
					InsertIndex = HitIndex + 1;
					break;
				}
			}
			Hits.Insert(HitEvent, InsertIndex);
		}
	};

	if (contact_listener.IsValid())
	{
		const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());
		UE_LOG(LogRecallPhysics, VeryVerbose, TEXT("[%d] Contacts: %d"), Frame, contact_listener->contact_events.Num());

		contact_listener->contact_events.Sort([](const FRecallContactEvent& lContact, const FRecallContactEvent& rContact)
		{
			if (lContact.BodyID1 == rContact.BodyID1)
			{
				if (lContact.BodyID2 == rContact.BodyID2)
				{
					if (lContact.SubShapeID1 == rContact.SubShapeID1)
					{
						return lContact.SubShapeID2 < rContact.SubShapeID2;
					}
					return lContact.SubShapeID1 < rContact.SubShapeID1;
				}
				return lContact.BodyID2 < rContact.BodyID2;
			}
			return lContact.BodyID1 < rContact.BodyID1;
		});
		
		ParallelFor(contact_listener->contact_events.Num(), [&](int32 Index)
			{
				const uint32 HitID = Index * 2 + 1; // Start at 1
				const FRecallContactEvent& ContactEvent = contact_listener->contact_events[Index];
				TryPushHitEvent(HitID, ContactEvent.BodyID1, ContactEvent.BodyID2, ContactEvent.Vel1, ContactEvent.Vel2,
					ContactEvent.ImpactPoint2, ContactEvent.ImpactNormal2, ContactEvent.bIsStatic2, ContactEvent.bIsSensor2);
				TryPushHitEvent(HitID + 1, ContactEvent.BodyID2, ContactEvent.BodyID1, ContactEvent.Vel2, ContactEvent.Vel1,
					ContactEvent.ImpactPoint1, ContactEvent.ImpactNormal1, ContactEvent.bIsStatic1, ContactEvent.bIsSensor1);
			}
		);

		contact_listener->contact_events.Reset();
	}
#endif // WITH_JOLT_PHYSICS

#if RECALL_DESYNC_LOG
	TArray<FRecallPhysicsBodyHandle> BodyHandles;
	HitEvents.GenerateKeyArray(BodyHandles);

	BodyHandles.Sort([](const FRecallPhysicsBodyHandle& lBodyHandle, const FRecallPhysicsBodyHandle& rBodyHandle)
		{
			return lBodyHandle.SerialNumber < rBodyHandle.SerialNumber;
		}
	);

	for (const FRecallPhysicsBodyHandle& BodyHandle : BodyHandles)
	{
		const TArray<FRecallPhysicsHitEvent>& Hits = HitEvents[BodyHandle];

		RECALL_DESYNC_LOG_CONTEXT(this, FString::Printf(TEXT("Body<%d> hit count: %d, "), BodyHandle.SerialNumber, Hits.Num()));
		for (const FRecallPhysicsHitEvent& Hit : Hits)
		{
			RECALL_DESYNC_LOG_CONTEXT(this, FString::Printf(TEXT("* ColliderBody<%d>, HitWorldLocation: %s, ImpactWorldNormal: %s, WorldVelocityAtImpact: %s"), 
				Hit.ColliderBodyHandle.SerialNumber, *Hit.ImpactWorldLocation.ToString(), *Hit.ImpactWorldNormal.ToString(), *Hit.WorldVelocityAtImpact.ToString()));
		}
	}
#endif // RECALL_DESYNC_LOG
}

TArray<FRecallPhysicsHitEvent> URecallPhysicsSubsystem::GetHitEvents(const FRecallPhysicsBodyHandle& Handle) const
{
	FScopeLock Lock(&HitEventGuard);
	return HitEvents.FindRef(Handle);
}

bool URecallPhysicsSubsystem::HasHitEvent(const FRecallPhysicsBodyHandle& Handle) const
{
	FScopeLock Lock(&HitEventGuard);
	return HitEvents.Contains(Handle);
}

void URecallPhysicsSubsystem::ResetHitEvents()
{
	FScopeLock Lock(&HitEventGuard);
	HitEvents.Reset();
}

int32 URecallPhysicsSubsystem::GetNumContactEvents() const
{
#if WITH_JOLT_PHYSICS
	if (contact_listener.IsValid())
	{
		return contact_listener->contact_events.Num();
	}
#endif // WITH_JOLT_PHYSICS
	return 0;
}

TWeakObjectPtr<const UJPRPhysicsLayerDataAsset> URecallPhysicsSubsystem::GetPhysicsLayer() const
{
	return PhysicsLayer;
}

void URecallPhysicsSubsystem::CreateStaticShape_Internal(const FInstancedStruct& Shape,
	const FVector& Location, const FQuat& Rotation,
	const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass, float Friction)
{
	checkf(!Recall::Simulation::Utils::HasSimulationStarted(this),
		TEXT("%hs Cannot create static body after simulation has started"), __FUNCTION__);

	CreateMutableStaticShape_Internal(Shape, Location, Rotation, FactoryClass, Friction);
}

FRecallPhysicsBodyHandle URecallPhysicsSubsystem::CreateMutableStaticShape_Internal(const FInstancedStruct& Shape,
	const FVector& Location, const FQuat& Rotation,
	const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass, float Friction)
{
	FRecallPhysicsBodyParameters Params;
	Params.MotionType = ERecallPhysicsMotionType::Static;
	Params.Friction = Friction;
	Params.bAllowDynamicOrKinematic = true;
	if (PhysicsLayer)
	{
		Params.Layer = PhysicsLayer->GetStaticLayerHandle();
	}

	constexpr FMassEntityHandle StaticDummyEntity;
	const FRecallPhysicsBodyHandle Handle = FRecallPhysicsBodyHandle{ ++SerialNumberGenerator };

	const URecallPhysicsObjectFactory* Factory = CreateShapeFactory(FactoryClass);
	const TSharedPtr<FRecallPhysicsBody> Body = Factory->BuildPhysicsObject(Handle.SerialNumber, Shape, Params);
	if (Body.IsValid())
	{
		Body->SetPositionAndRotation(Location, Rotation);
		Body->Activate();
	}

	BodyRefMap.Add(Handle, FRecallPhysicsBodyRef{ StaticDummyEntity, Body, Shape, Params });
	BodyHandleMap.Add(Handle.SerialNumber, Handle);

	return Handle;
}

URecallPhysicsObjectFactory* URecallPhysicsSubsystem::CreateShapeFactory(
	const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass)
{
	if (IsInGameThread())
	{
		return NewObject<URecallPhysicsObjectFactory>(this, FactoryClass);
	}
	else
	{
		// Guard against GC interference
		FGCScopeGuard GCGuard;
		URecallPhysicsObjectFactory* Factory = NewObject<URecallPhysicsObjectFactory>(this, FactoryClass);

		// Clear the async flag
		AsyncTask(ENamedThreads::GameThread, [Factory]() mutable {
			Factory->ClearInternalFlags(EInternalObjectFlags::Async);
		});

		return Factory;
	}
}

void URecallPhysicsSubsystem::CreateShape_Internal(const FMassEntityHandle& Entity, const FInstancedStruct& Shape,
	const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass, const FRecallPhysicsBodyParameters& Params,
	FRecallPhysicsBodyHandle& Handle)
{
	// checkf(Entity.IsSet(),
	// 	TEXT("%hs Use CreateStaticShape to create static shape without an entity"), __FUNCTION__);

	bool bRestoreBody = true;

	if (Params.IsStatic())
	{
		if (EntitySystem->IsStaticEntity(Entity))
		{
			checkf(!Handle.IsSet(),
				TEXT("%hs Static entities should not be restored"), __FUNCTION__);
		
			bRestoreBody = false;
		}
		else
		{
			UE_LOG(LogRecallPhysics, Log,
				TEXT("%hs Entities with static collider can be made static to improve performance (unless it should explicitly be mutable)"), __FUNCTION__);
		}
	}
	
	FScopeLock Lock(&DataGuard);
	
	if (!Handle.IsValid())
	{
		Handle = FRecallPhysicsBodyHandle{ ++SerialNumberGenerator };
	}

	const URecallPhysicsObjectFactory* Factory = CreateShapeFactory(FactoryClass);	
	const TSharedPtr<FRecallPhysicsBody> Body = Factory->BuildPhysicsObject(Handle.SerialNumber, Shape, Params);

	BodyRefMap.Add(Handle, FRecallPhysicsBodyRef{ Entity, Body, Shape, Params, bRestoreBody });
	BodyHandleMap.Add(Handle.SerialNumber, Handle);
}

void URecallPhysicsSubsystem::CheckSimulationProcessingPhase() const
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);
}

void URecallPhysicsSubsystem::CreateFixedConstrain(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	const TWeakPtr<const FRecallPhysicsBody> Body1 = GetBody(Handle1);
	const TWeakPtr<const FRecallPhysicsBody> Body2 = GetBody(Handle2);

	if (!Body1.IsValid() || !Body2.IsValid())
	{
		UE_LOG(LogRecallPhysics, Warning,
			TEXT("%hs One of the bodies to constrain is not valid"), __FUNCTION__);
		return;
	}

#if WITH_JOLT_PHYSICS
	TwoBodyConstraint* Constrain = nullptr;
	
	{
		FixedConstraintSettings ConstraintSettings;
		ConstraintSettings.mAutoDetectPoint = true;

		BodyID constraint_bodies[] = { BodyID(Body1.Pin()->GetBodyID()), BodyID(Body2.Pin()->GetBodyID()) };
		BodyLockMultiWrite lock(GetPhysicsSystem().GetBodyLockInterface(), constraint_bodies, 2);

		Body* body1 = lock.GetBody(0);
		Body* body2 = lock.GetBody(1);

		if (ensure(body1 != nullptr) && ensure(body2 != nullptr))
		{
			Constrain = ConstraintSettings.Create(*body1, *body2);
		}
	}

	if (Constrain == nullptr)
	{
		UE_LOG(LogRecallPhysics, Warning,
			TEXT("%hs Failed to created fixed constrain"), __FUNCTION__);
		return;
	}

	GetPhysicsSystem().AddConstraint(Constrain);

	if (Body1.Pin()->IsEnabled() && Body2.Pin()->IsEnabled())
	{
		GetBodyInterface().ActivateConstraint(Constrain);
	}

	AddConstrainRef_Internal(Handle1, Handle2);
#endif // WITH_JOLT_PHYSICS
}

void URecallPhysicsSubsystem::RemoveAllConstrains(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
#if WITH_JOLT_PHYSICS
	const TArray<BodyID> constraint_bodies = { BodyID(Handle1.SerialNumber), BodyID(Handle2.SerialNumber) };

	Constraints constraints = GetPhysicsSystem().GetConstraints();

	for (Constraint* constrain : constraints)
	{
		const TwoBodyConstraint* two_body_constraint = static_cast<TwoBodyConstraint*>(constrain);
		if (two_body_constraint == nullptr)
		{
			continue;
		}

		const Body* body1 = two_body_constraint->GetBody1();
		const Body* body2 = two_body_constraint->GetBody2();

		if (ensure(body1 != nullptr && body2 != nullptr) &&
			constraint_bodies.Contains(body1->GetID()) && constraint_bodies.Contains(body2->GetID()))
		{
			GetPhysicsSystem().RemoveConstraint(constrain);
		}
	}
	
	RemoveConstrainRef_Internal(Handle1, Handle2);
#endif // WITH_JOLT_PHYSICS
}

void URecallPhysicsSubsystem::AddConstrainRef_Internal(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	const FRecallPhysicsConstrainHandle NewHandle(Handle1, Handle2);
	check(!ConstrainRefs.Contains(NewHandle));
	ConstrainRefs.Add(NewHandle);
}

void URecallPhysicsSubsystem::RemoveConstrainRef_Internal(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	const FRecallPhysicsConstrainHandle Handle(Handle1, Handle2);
	check(ConstrainRefs.Contains(Handle));
	ConstrainRefs.Remove(Handle);
}

void URecallPhysicsSubsystem::ReleaseBody(const FRecallPhysicsBodyHandle& Handle)
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);

	if (!Handle.IsValid()) return;

	FScopeLock Lock(&DataGuard);
	ReleaseBody_Internal(Handle);
}

void URecallPhysicsSubsystem::ReleaseBody_Internal(const FRecallPhysicsBodyHandle& Handle, bool bCleanUp /*= false*/)
{
	FRecallPhysicsBodyRef BodyRef;
	if (BodyRefMap.RemoveAndCopyValue(Handle, BodyRef))
	{
		if (ensure(BodyRef.Body.IsValid()))
		{
			BodyHandleMap.Remove(BodyRef.Body->GetBodyID());
			BodyRef.Body->ReleasePhysicsObject();
		}
	}
}

TWeakPtr<FRecallPhysicsBody> URecallPhysicsSubsystem::GetMutableBody(const FRecallPhysicsBodyHandle& Handle)
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);

	if (Handle.IsValid())
	{
		if (const FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
		{
			return BodyRef->Body.ToWeakPtr();
		}
	}

	return nullptr;
}

TWeakPtr<const FRecallPhysicsBody> URecallPhysicsSubsystem::GetBody(const FRecallPhysicsBodyHandle& Handle) const
{
	if (Handle.IsValid())
	{
		if (const FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
		{
			return StaticCastWeakPtr<const FRecallPhysicsBody>(TWeakPtr<FRecallPhysicsBody>(BodyRef->Body));
		}
	}

	return nullptr;
}

/*
bool URecallPhysicsSubsystem::CastShape(const FRecallPhysicsBodyHandle& Handle, const FIntVector& Direction, TArray<FRecallPhysicsCastShapeResult>& OutHits) const
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_CastShape);

	const TWeakPtr<const FRecallPhysicsBody> Body = GetBody(Handle);
	if (!Body.IsValid())
	{
		return false;
	}

#if WITH_JOLT_PHYSICS
	const BodyID body_id(Body.Pin()->GetBodyID());
	const ShapeRefC ShapeRef = GetBodyInterface().GetShape(body_id);
	const Shape* Shape = ShapeRef.GetPtr();
	if (Shape == nullptr)
	{
		return false;
	}

	FVector WorldPos = FVector::ZeroVector;
	Body.Pin()->GetPosition(WorldPos);

	check(ViewPlaneSystem.IsValid());
	const FVector WorldDirection = ViewPlaneSystem->GetUnrealVectorAlongViewPlane(WorldPos, RecallToUnreal(Direction));
	const FVector PhysicsDirection = UnrealToJoltPhysics(WorldDirection);

	const RMat44 start = GetBodyInterface().GetCenterOfMassTransform(body_id);
	const Vec3 dir(PhysicsDirection.X, PhysicsDirection.Y, PhysicsDirection.Z);

	// Settings for the cast
	ShapeCastSettings settings;
	settings.mBackFaceModeTriangles = EBackFaceMode::IgnoreBackFaces;
	settings.mBackFaceModeConvex = EBackFaceMode::IgnoreBackFaces;
	settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
	settings.mUseShrunkenShapeAndConvexRadius = true;
	settings.mReturnDeepestPoint = false;

	// Cast shape
	const TArray<ObjectLayer> Layers = { (ObjectLayer)ERecallPhysicsLayer::STATIC, (ObjectLayer)ERecallPhysicsLayer::EDGE_WALL_SIDE };
	const RShapeCast shape_cast(Shape, Vec3::sReplicate(1.0f), start, dir);
	FRecallPhysicsCollector collector(GetPhysicsSystem(), body_id, shape_cast, Layers);
	GetPhysicsSystem().GetNarrowPhaseQuery().CastShape(shape_cast, settings, start.GetTranslation(), collector);

	OutHits.Reserve(OutHits.Num() + collector.mHitEvents.Num());

	for (const FRecallPhysicsHit& hit_event : collector.mHitEvents)
	{
		if (ensure(hit_event.mBody != nullptr) == false) continue;

		const uint32 HitBodyID = hit_event.mBody->GetID().GetIndexAndSequenceNumber();

		// Check walls and if they should trigger hit event or not.
		const TSharedPtr<FRecallPhysicsBody> HitWallBody = WallBodies.FindRef(HitBodyID);
		if (HitWallBody.IsValid() && HitWallBody->DoesTriggerHitEvents() == false)
		{
			continue;
		}

		FRecallPhysicsCastShapeResult Hit;

		if (const FRecallPhysicsBodyHandle* HitBodyHandle = BodyHandleMap.Find(HitBodyID))
		{
			const TWeakPtr<const FRecallPhysicsBody> HitBody = GetBody(*HitBodyHandle);
			if (ensure(HitBody.IsValid()) && HitBody.Pin()->DoesTriggerHitEvents() == false)
			{
				continue;
			}

			Hit.HitBodyHandle = *HitBodyHandle;
		}

		const FVector PhysicsHitPos(hit_event.mContactPosition.GetX(), hit_event.mContactPosition.GetY(), hit_event.mContactPosition.GetZ());
		const FVector WorldHitPos = JoltPhysicsToUnreal(PhysicsHitPos);

		Hit.HitLocation = ViewPlaneSystem->GetSimLocationFromWorldPosition(WorldHitPos);

		const FVector PhysicsNormal(hit_event.mContactNormal.GetX(), hit_event.mContactNormal.GetY(), hit_event.mContactNormal.GetZ());
		const FVector WorldNormal = JoltPhysicsToUnreal(PhysicsNormal) / JoltPhysicsToUnrealUnitScale;

		const FIntVector SimNormal = ViewPlaneSystem->GetSimVectorFromWorldVector(WorldPos, WorldNormal);

		Hit.HitNormal.X = FMath::RoundToInt((double)SimNormal.X / (double)UnrealToRecallUnitScale);
		Hit.HitNormal.Y = FMath::RoundToInt((double)SimNormal.Y / (double)UnrealToRecallUnitScale);
		Hit.HitNormal.Z = FMath::RoundToInt((double)SimNormal.Z / (double)UnrealToRecallUnitScale);

		const double SimPenetration = (double)hit_event.mPenetrationDepth * JoltPhysicsToUnrealUnitScale * UnrealToJoltPhysicsUnitScale;

		Hit.HitPenetration.X = -(double)SimNormal.X * SimPenetration / (double)UnrealToRecallUnitScale;
		Hit.HitPenetration.Y = -(double)SimNormal.Y * SimPenetration / (double)UnrealToRecallUnitScale;
		Hit.HitPenetration.Z = -(double)SimNormal.Z * SimPenetration / (double)UnrealToRecallUnitScale;

		OutHits.Add(Hit);
	}

	return OutHits.Num() > 0;
#endif // WITH_JOLT_PHYSICS
}
*/

#if WITH_JOLT_PHYSICS
void URecallPhysicsSubsystem::SaveState(JPH::StateRecorder& OutState)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsSubsystem::SaveState"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_SaveState);

	GetPhysicsSystem().SaveState(OutState, EStateRecorderState::All, StateRecorderFilter.Get());

	TArray<FRecallPhysicsBodyHandle> BodyHandles;
	BodyRefMap.GenerateKeyArray(BodyHandles);

	BodyHandles.Sort([](const FRecallPhysicsBodyHandle& lHandle, const FRecallPhysicsBodyHandle& rHandle)
		{
			return lHandle.SerialNumber < rHandle.SerialNumber;
		}
	);

	for (const FRecallPhysicsBodyHandle& BodyHandle : BodyHandles)
	{
		const FRecallPhysicsBodyRef& BoydRef = BodyRefMap[BodyHandle];
		if (ensure(BoydRef.Body.IsValid()))
		{
			BoydRef.Body->SaveState(OutState);
		}
	}
}

bool URecallPhysicsSubsystem::RestoreState(JPH::StateRecorder& InState)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsSubsystem::RestoreState"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_RestoreState);

	if (ensure(GetPhysicsSystem().RestoreState(InState, StateRecorderFilter.Get())))
	{
		TArray<FRecallPhysicsBodyHandle> BodyHandles;
		BodyRefMap.GenerateKeyArray(BodyHandles);

		BodyHandles.Sort([](const FRecallPhysicsBodyHandle& lHandle, const FRecallPhysicsBodyHandle& rHandle)
			{
				return lHandle.SerialNumber < rHandle.SerialNumber;
			}
		);

		for (const FRecallPhysicsBodyHandle& BodyHandle : BodyHandles)
		{
			const FRecallPhysicsBodyRef& BoydRef = BodyRefMap[BodyHandle];
			if (ensure(BoydRef.Body.IsValid()))
			{
				BoydRef.Body->RestoreState(InState);
			}
		}

		return true;
	}

	return false;
}
#endif // WITH_JOLT_PHYSICS

void URecallPhysicsSubsystem::CreatePhysicsSystem()
{
#if WITH_JOLT_PHYSICS
	// Install callbacks
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	body_activation_listener = MakeShared<FRecallBodyActivationListener>();

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	contact_listener = MakeShared<FRecallContactListener>();

	UJPRPhysicsSubsystem::CreatePhysicsSystem(*PhysicsLayer, body_activation_listener.Get(), contact_listener.Get(),
		MakeShared<FRecallStateRecorderFilter>(*this));
#endif // WITH_JOLT_PHYSICS
}

void URecallPhysicsSubsystem::OnActorsInitialized(const FActorsInitializedParams& Params)
{
	LandscapeActor = Cast<ALandscape>(UGameplayStatics::GetActorOfClass(this, ALandscape::StaticClass()));
}

void URecallPhysicsSubsystem::SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, uint16 Layer)
{
	FScopeLock Lock(&DataGuard);
	if (FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (BodyRef->LayerOverride.IsValid() && *BodyRef->LayerOverride.Get() == Layer)
		{
			return;
		}

		BodyRef->LayerOverride = MakeUnique<uint16>(Layer);
		
		if (BodyRef->Body.IsValid())
		{
#if WITH_JOLT_PHYSICS
			GetBodyInterface().SetObjectLayer(BodyID(BodyRef->Body->GetBodyID()), (ObjectLayer)Layer);
#endif // WITH_JOLT_PHYSICS
		}
	}
}

bool URecallPhysicsSubsystem::HasLayerOverride(const FRecallPhysicsBodyHandle& Handle) const
{
	FScopeLock Lock(&DataGuard);
	if (const FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		return BodyRef->LayerOverride.IsValid();
	}

	return false;
}

void URecallPhysicsSubsystem::ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle)
{
	FScopeLock Lock(&DataGuard);
	if (FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (!BodyRef->LayerOverride.IsValid())
		{
			return;
		}

		// Reset our layer.
		BodyRef->LayerOverride.Reset();

		if (BodyRef->Body.IsValid())
		{
			const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(BodyRef->Params.Layer);
#if WITH_JOLT_PHYSICS
			GetBodyInterface().SetObjectLayer(BodyID(BodyRef->Body->GetBodyID()), (ObjectLayer)Layer);
#endif // WITH_JOLT_PHYSICS
		}
	}
}

void URecallPhysicsSubsystem::SetMotionType(const FRecallPhysicsBodyHandle& Handle, ERecallPhysicsMotionType MotionType, ERecallPhysicsActivation ActivationMode /*= ERecallPhysicsActivation::Activate*/)
{
	FScopeLock Lock(&DataGuard);
	if (FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (BodyRef->Body.IsValid())
		{
#if WITH_JOLT_PHYSICS
			const BodyID bodyID(BodyRef->Body->GetBodyID());

			// Can not edit motion type for static bodies except if they are allowed to change their motion type
			if (GetBodyInterface().GetMotionType(bodyID) == EMotionType::Static && !BodyRef->Params.bAllowDynamicOrKinematic)
			{
				return;
			}

			if (GetBodyInterface().GetMotionType(bodyID) == (EMotionType)MotionType)
			{
				return;
			}

			if (MotionType == BodyRef->Params.MotionType) // Default
			{
				BodyRef->MotionTypeOverride.Reset();
			}
			else
			{
				BodyRef->MotionTypeOverride = MakeUnique<ERecallPhysicsMotionType>(MotionType);
			}

			GetBodyInterface().SetMotionType(bodyID, (EMotionType)MotionType, (EActivation)ActivationMode);
#endif // WITH_JOLT_PHYSICS
		}
	}
}

void URecallPhysicsSubsystem::ResetMotionType(const FRecallPhysicsBodyHandle& Handle)
{
	FScopeLock Lock(&DataGuard);
	if (FRecallPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (BodyRef->MotionTypeOverride.IsValid())
		{
			SetMotionType(Handle, BodyRef->Params.MotionType);
		}
	}
}

/*
void URecallPhysicsSubsystem::RunSample()
{
#if WITH_JOLT_PHYSICS
	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	BodyInterface& body_interface = GetBodyInterface();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape). 
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));

	// Create the shape
	ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, (ObjectLayer)0);

	// Create the actual rigid body
	Body* floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

	// Add it to the world
	body_interface.AddBody(floor->GetID(), EActivation::DontActivate);

	// Now create a dynamic body to bounce on the floor
	// Note that this uses the shorthand version of creating and adding a body to the world
	BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, (ObjectLayer)1);
	BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

	// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
	// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
	body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	PhysicsSystem->OptimizeBroadPhase();

	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	const float cDeltaTime = 1.0f / 60.0f;

	// Now we're ready to simulate the body, keep simulating until it goes to sleep
	uint step = 0;
	while (body_interface.IsActive(sphere_id))
	{
		// Next step
		++step;

		// Output current position and velocity of the sphere
		RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
		Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
		UE_LOG(LogRecallPhysics, Log, TEXT("Step %d: Position = (%.2f, %.2f, %.2f), Velocity = (%.2f, %.2f, %.2f)"),
			step, position.GetX(), position.GetY(), position.GetZ(), velocity.GetX(), velocity.GetY(), velocity.GetZ());

		// Step the world
		PhysicsSystem->Update(cDeltaTime, CollisionSteps, IntegrationSubSteps, TempAllocator.Get(), JobSystem.Get());
	}

	// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
	body_interface.RemoveBody(sphere_id);

	// Destroy the sphere. After this the sphere ID is no longer valid.
	body_interface.DestroyBody(sphere_id);

	// Remove and destroy the floor
	body_interface.RemoveBody(floor->GetID());
	body_interface.DestroyBody(floor->GetID());
#endif // WITH_JOLT_PHYSICS
}
*/
