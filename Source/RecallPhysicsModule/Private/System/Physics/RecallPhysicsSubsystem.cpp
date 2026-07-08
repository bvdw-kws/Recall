// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Physics/RecallPhysicsSubsystem.h"

#include "Async/ParallelFor.h"
#include "Desync/RecallDesyncLog.h"
#include "Kismet/GameplayStatics.h"
#include "Landscape.h"
#include "RecallPhysicsSnapshot.h"
#include "Physics/JPRPhysicsBody.h"
#include "Physics/JPRPhysicsObjectFactory.h"
#include "Physics/JPRPhysicsLayerDataAsset.h"
#include "Physics/JPRPhysicsMath.h"
#include "Physics/RecallPhysicsObjects.h"
#include "Physics/JPRPhysicsShapeTypes.h"
#include "Settings/JPRPhysicsSettings.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "Utility/Physics/RecallPhysicsUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"
#include "Utility/Slowmo/RecallSlowmoUtils.h"
#include "World/RecallWorldSettings.h"

DEFINE_LOG_CATEGORY(LogRecallPhysics);

void URecallPhysicsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallEntitySubsystem>();

	EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(GetWorld());
	
	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::OnActorsInitialized);
}

void URecallPhysicsSubsystem::Deinitialize()
{
	EntitySystem.Reset();

	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);

	ReleasePhysicsObjects();
	
	Super::Deinitialize();
}

bool URecallPhysicsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return UWorldSubsystem::ShouldCreateSubsystem(Outer);
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
				TArray<FRecallPhysicsBodyHandle> BodyHandles;
				BodyRefMap.GenerateKeyArray(BodyHandles);
				BodyHandles.Sort([](const FRecallPhysicsBodyHandle& Left, const FRecallPhysicsBodyHandle& Right)
				{
					return Left.SerialNumber < Right.SerialNumber;
				});

				TArray<TSharedPtr<FJPRPhysicsBody>> Bodies;
				Bodies.Reserve(BodyHandles.Num());
				for (const FRecallPhysicsBodyHandle& BodyHandle : BodyHandles)
				{
					Bodies.Add(BodyRefMap.FindChecked(BodyHandle).Body);
				}
				Snasphot.Data = SavePhysicsState(Bodies);
			}
			break;

			case 1: // Save Bodies
			{
				Snasphot.Bodies.Reserve(BodyRefMap.Num());

				for (const TPair<FRecallPhysicsBodyHandle, FJPRPhysicsBodyRef>& PhysicsBodyRef : BodyRefMap)
				{
					if (!PhysicsBodyRef.Value.bRestoreBody)
					{
						continue;
					}
					
					const FJPRPhysicsBodySnapshot BodySnapshot = TakeBodySnapshot(PhysicsBodyRef.Key);
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
		const FJPRPhysicsBodyRef& OldBodyRef = BodyRefMap[OldHandle];
		if (!OldBodyRef.bRestoreBody)
		{
			checkf(!NewHandles.Contains(OldHandle),
				TEXT("%hs Should not save bodies that must not be restored"), __FUNCTION__);
			continue;
		}
		
		// Keep existing bodies so they can be restored.
		if (NewHandles.Contains(OldHandle))
		{
			const FJPRPhysicsBodyRef& NewBodySnapshot = BodyRefMap[OldHandle];
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
		const FJPRPhysicsBodySnapshot& BodySnapshot = InData->Bodies[NewHandle];
		const FJPRPhysicsShape& Shape = BodySnapshot.Shape.Get<FJPRPhysicsShape>();

		CreateShape_Internal(BodySnapshot.Entity, BodySnapshot.Shape, Shape.FactoryClass, BodySnapshot.Params, NewHandle);
	}

	// Re-apply body settings
	for (const TPair<FRecallPhysicsBodyHandle, FJPRPhysicsBodySnapshot>& RestorePair : InData->Bodies)
	{
		const FRecallPhysicsBodyHandle& Handle = RestorePair.Key;

		const FJPRPhysicsBodyRef& BodyRef = BodyRefMap.FindChecked(Handle);
		const TSharedPtr<FJPRPhysicsBody>& Body = BodyRef.Body;
		check(Body.IsValid());
		
		const FJPRPhysicsBodySnapshot& BodySnapshot = RestorePair.Value;

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

	TArray<FRecallPhysicsBodyHandle> BodyHandles;
	BodyRefMap.GenerateKeyArray(BodyHandles);
	BodyHandles.Sort([](const FRecallPhysicsBodyHandle& Left, const FRecallPhysicsBodyHandle& Right)
	{
		return Left.SerialNumber < Right.SerialNumber;
	});

	TArray<TSharedPtr<FJPRPhysicsBody>> Bodies;
	Bodies.Reserve(BodyHandles.Num());
	for (const FRecallPhysicsBodyHandle& BodyHandle : BodyHandles)
	{
		Bodies.Add(BodyRefMap.FindChecked(BodyHandle).Body);
	}
	RestorePhysicsState(InData->Data, Bodies);
}

FJPRPhysicsBodySnapshot URecallPhysicsSubsystem::TakeBodySnapshot(const FRecallPhysicsBodyHandle& Handle) const
{
	const FJPRPhysicsBodyRef& PhysicsBodyRef = BodyRefMap.FindChecked(Handle);
	
	FJPRPhysicsBodySnapshot BodySnapshot;
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
	CheckPhysicsAccess();
	
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
	if (HasPhysicsSystem())
	{
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_TickPhysics);
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallPhysicsSubsystem::TickPhysics Update")));

		const UJPRPhysicsSettings* PhysicsSettings = GetDefault<UJPRPhysicsSettings>();
		const float TimeDilatation = Recall::Slowmo::Utils::GetTimeDilatation(this);
		StartPhysicsSimulation(DeltaTime, PhysicsSettings->CollisionSteps, TimeDilatation);
	}
}

bool URecallPhysicsSubsystem::ShouldGenerateHitEvent(const uint32 BodyID) const
{
	// Some custom colliders may not be registered in this map but still generate hit events
	if (const FRecallPhysicsBodyHandle* BodyHandlePtr = BodyHandleMap.Find(BodyID))
	{
		const FConstRecallPhysicsBodyView BodyPtr = GetBody(*BodyHandlePtr);
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
		if (const FJPRPhysicsBodyRef* BodyRefPtr = BodyRefMap.Find(*BodyHandlePtr))
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

	auto TryPushHitEvent = [&](uint32 HitID, const uint32 Body1ID, const uint32 Body2ID, const FVector& Velocity,
		const FVector& ImpactPoint, const FVector& ImpactNormal, bool bHitStatic, bool bHitSensor)
	{
		const FRecallPhysicsBodyHandle* Body1HandlePtr = BodyHandleMap.Find(Body1ID);

		if (Body1HandlePtr == nullptr || !GeneratesHitEventsBodyHandles.Contains(*Body1HandlePtr))
		{
			return;
		}

		if (ShouldGenerateHitEvent(Body2ID) == false)
		{
			return;
		}

		FRecallPhysicsHitEvent HitEvent;
		HitEvent.HitID = HitID;

		if (const FRecallPhysicsBodyHandle* Body2HandlePtr = BodyHandleMap.Find(Body2ID))
		{
			HitEvent.ColliderBodyHandle = *Body2HandlePtr;

			if (const FJPRPhysicsBodyRef* Body2Ref = BodyRefMap.Find(*Body2HandlePtr))
			{
				// Prevent the entity from detecting hit with itself
				const FJPRPhysicsBodyRef* Body1Ref = BodyRefMap.Find(*Body1HandlePtr);
				if (Body1Ref != nullptr && Body1Ref->Entity == Body2Ref->Entity)
				{
					return;
				}
				
				HitEvent.ColliderEntity = Body2Ref->Entity;
			}
		}

		HitEvent.ImpactWorldLocation = JoltPhysicsToUnreal(ImpactPoint);
		HitEvent.ImpactWorldNormal = JoltPhysicsToUnrealDirection(ImpactNormal);
		HitEvent.bHitStatic = bHitStatic;
		HitEvent.bHitSensor = bHitSensor;

		HitEvent.WorldVelocityAtImpact = JoltPhysicsToUnreal(Velocity);

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

	TArray<FJPRContactEvent> ContactEvents = ConsumeContactEvents();
	if (!ContactEvents.IsEmpty())
	{
		const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());
		UE_LOG(LogRecallPhysics, VeryVerbose, TEXT("[%d] Contacts: %d"), Frame, ContactEvents.Num());

		ContactEvents.Sort([](const FJPRContactEvent& lContact, const FJPRContactEvent& rContact)
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
		
		ParallelFor(ContactEvents.Num(), [&](int32 Index)
			{
				const uint32 HitID = Index * 2 + 1; // Start at 1
				const FJPRContactEvent& ContactEvent = ContactEvents[Index];
				TryPushHitEvent(HitID, ContactEvent.BodyID1, ContactEvent.BodyID2, ContactEvent.Velocity1,
					ContactEvent.ImpactPoint2, ContactEvent.ImpactNormal2, ContactEvent.bIsStatic2, ContactEvent.bIsSensor2);
				TryPushHitEvent(HitID + 1, ContactEvent.BodyID2, ContactEvent.BodyID1, ContactEvent.Velocity2,
					ContactEvent.ImpactPoint1, ContactEvent.ImpactNormal1, ContactEvent.bIsStatic1, ContactEvent.bIsSensor1);
			}
		);
	}

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
	return GetNumPendingContactEvents();
}

TWeakObjectPtr<const UJPRPhysicsLayerDataAsset> URecallPhysicsSubsystem::GetPhysicsLayer() const
{
	return PhysicsLayer;
}

void URecallPhysicsSubsystem::CreateStaticShape_Internal(const FInstancedStruct& Shape,
	const FVector& Location, const FQuat& Rotation,
	const TSubclassOf<UJPRPhysicsObjectFactory>& FactoryClass, float Friction)
{
	checkf(!Recall::Simulation::Utils::HasSimulationStarted(this),
		TEXT("%hs Cannot create static body after simulation has started"), __FUNCTION__);

	CreateMutableStaticShape_Internal(Shape, Location, Rotation, FactoryClass, Friction);
}

FRecallPhysicsBodyHandle URecallPhysicsSubsystem::CreateMutableStaticShape_Internal(const FInstancedStruct& Shape,
	const FVector& Location, const FQuat& Rotation,
	const TSubclassOf<UJPRPhysicsObjectFactory>& FactoryClass, float Friction)
{
	FJPRPhysicsBodyParameters Params;
	Params.MotionType = EJPRPhysicsMotionType::Static;
	Params.Friction = Friction;
	Params.bAllowDynamicOrKinematic = true;
	if (PhysicsLayer)
	{
		Params.Layer = PhysicsLayer->GetStaticLayerHandle();
	}

	constexpr FMassEntityHandle StaticDummyEntity;
	const FRecallPhysicsBodyHandle Handle = FRecallPhysicsBodyHandle{ ++SerialNumberGenerator };

	const TSharedPtr<FJPRPhysicsBody> Body = CreateBodyFromShape(Handle.SerialNumber, Shape, FactoryClass, Params);
	if (Body.IsValid())
	{
		Body->SetPositionAndRotation(Location, Rotation);
		Body->Activate();
	}

	BodyRefMap.Add(Handle, FJPRPhysicsBodyRef{ StaticDummyEntity, Body, Shape, Params });
	BodyHandleMap.Add(Handle.SerialNumber, Handle);

	return Handle;
}

void URecallPhysicsSubsystem::CreateShape_Internal(const FMassEntityHandle& Entity, const FInstancedStruct& Shape,
	const TSubclassOf<UJPRPhysicsObjectFactory>& FactoryClass, const FJPRPhysicsBodyParameters& Params,
	FRecallPhysicsBodyHandle& Handle)
{
	CheckPhysicsAccess();
	
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

	const TSharedPtr<FJPRPhysicsBody> Body = CreateBodyFromShape(Handle.SerialNumber, Shape, FactoryClass, Params);

	BodyRefMap.Add(Handle, FJPRPhysicsBodyRef{ Entity, Body, Shape, Params, bRestoreBody });
	BodyHandleMap.Add(Handle.SerialNumber, Handle);
}

int32 URecallPhysicsSubsystem::GetNumPendingContactEvents() const
{
	return Super::GetNumPendingContactEvents();
}

void URecallPhysicsSubsystem::SetBodyObjectLayer(const uint32 BodyID, const uint16 Layer)
{
	Super::SetBodyObjectLayer(BodyID, Layer);
}

EJPRPhysicsMotionType URecallPhysicsSubsystem::GetBodyMotionType(const uint32 BodyID) const
{
	return Super::GetBodyMotionType(BodyID);
}

void URecallPhysicsSubsystem::SetBodyMotionType(const uint32 BodyID, const EJPRPhysicsMotionType MotionType,
	const EJPRPhysicsActivation ActivationMode)
{
	Super::SetBodyMotionType(BodyID, MotionType, ActivationMode);
}

bool URecallPhysicsSubsystem::CreateFixedConstraint(const uint32 BodyID1, const uint32 BodyID2, const bool bActivate)
{
	return Super::CreateFixedConstraint(BodyID1, BodyID2, bActivate);
}

void URecallPhysicsSubsystem::RemoveFixedConstraints(const uint32 BodyID1, const uint32 BodyID2)
{
	Super::RemoveFixedConstraints(BodyID1, BodyID2);
}

TArray<FJPRContactEvent> URecallPhysicsSubsystem::ConsumeContactEvents()
{
	return Super::ConsumeContactEvents();
}

void URecallPhysicsSubsystem::CheckSimulationProcessingPhase() const
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);
}

void URecallPhysicsSubsystem::CreateFixedConstrain(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	const FConstRecallPhysicsBodyView Body1 = GetBody(Handle1);
	const FConstRecallPhysicsBodyView Body2 = GetBody(Handle2);

	if (!Body1.IsValid() || !Body2.IsValid())
	{
		UE_LOG(LogRecallPhysics, Warning,
			TEXT("%hs One of the bodies to constrain is not valid"), __FUNCTION__);
		return;
	}

	if (!CreateFixedConstraint(Body1.Pin()->GetBodyID(), Body2.Pin()->GetBodyID(), Body1.Pin()->IsEnabled() && Body2.Pin()->IsEnabled()))
	{
		UE_LOG(LogRecallPhysics, Warning,
			TEXT("%hs Failed to created fixed constrain"), __FUNCTION__);
		return;
	}

	AddConstrainRef_Internal(Handle1, Handle2);
}

void URecallPhysicsSubsystem::RemoveAllConstrains(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	RemoveFixedConstraints(Handle1.SerialNumber, Handle2.SerialNumber);
	RemoveConstrainRef_Internal(Handle1, Handle2);
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
	CheckPhysicsAccess();
	
	FJPRPhysicsBodyRef BodyRef;
	if (BodyRefMap.RemoveAndCopyValue(Handle, BodyRef))
	{
		if (ensure(BodyRef.Body.IsValid()))
		{
			BodyHandleMap.Remove(BodyRef.Body->GetBodyID());
			BodyRef.Body->ReleasePhysicsObject();
		}
	}
}

FRecallPhysicsBodyView URecallPhysicsSubsystem::GetMutableBody(const FRecallPhysicsBodyHandle& Handle)
{
	CheckPhysicsAccess();
	
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);

	if (Handle.IsValid())
	{
		if (const FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
		{
			return FRecallPhysicsBodyView(BodyRef->Body.ToWeakPtr());
		}
	}

	return nullptr;
}

FConstRecallPhysicsBodyView URecallPhysicsSubsystem::GetBody(const FRecallPhysicsBodyHandle& Handle) const
{
	CheckPhysicsAccess();
	
	if (Handle.IsValid())
	{
		if (const FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
		{
			return FConstRecallPhysicsBodyView(BodyRef->Body.ToWeakPtr());
		}
	}

	return nullptr;
}

void URecallPhysicsSubsystem::OnActorsInitialized(const FActorsInitializedParams& Params)
{
	LandscapeActor = Cast<ALandscape>(UGameplayStatics::GetActorOfClass(this, ALandscape::StaticClass()));
}

void URecallPhysicsSubsystem::SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, uint16 Layer)
{
	CheckPhysicsAccess();
	
	FScopeLock Lock(&DataGuard);
	if (FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (BodyRef->LayerOverride.IsValid() && *BodyRef->LayerOverride.Get() == Layer)
		{
			return;
		}

		BodyRef->LayerOverride = MakeUnique<uint16>(Layer);
		
		if (BodyRef->Body.IsValid())
		{
			SetBodyObjectLayer(BodyRef->Body->GetBodyID(), Layer);
		}
	}
}

bool URecallPhysicsSubsystem::HasLayerOverride(const FRecallPhysicsBodyHandle& Handle) const
{
	FScopeLock Lock(&DataGuard);
	if (const FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		return BodyRef->LayerOverride.IsValid();
	}

	return false;
}

void URecallPhysicsSubsystem::ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle)
{
	CheckPhysicsAccess();
	
	FScopeLock Lock(&DataGuard);
	if (FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
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
			SetBodyObjectLayer(BodyRef->Body->GetBodyID(), Layer);
		}
	}
}

void URecallPhysicsSubsystem::SetMotionType(const FRecallPhysicsBodyHandle& Handle, EJPRPhysicsMotionType MotionType, EJPRPhysicsActivation ActivationMode /*= EJPRPhysicsActivation::Activate*/)
{
	FScopeLock Lock(&DataGuard);
	if (FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (BodyRef->Body.IsValid())
		{
			const uint32 BodyID = BodyRef->Body->GetBodyID();
			// Can not edit motion type for static bodies except if they are allowed to change their motion type
			if (GetBodyMotionType(BodyID) == EJPRPhysicsMotionType::Static && !BodyRef->Params.bAllowDynamicOrKinematic)
			{
				return;
			}

			if (GetBodyMotionType(BodyID) == MotionType)
			{
				return;
			}

			if (MotionType == BodyRef->Params.MotionType) // Default
			{
				BodyRef->MotionTypeOverride.Reset();
			}
			else
			{
				BodyRef->MotionTypeOverride = MakeUnique<EJPRPhysicsMotionType>(MotionType);
			}

			SetBodyMotionType(BodyID, MotionType, ActivationMode);
		}
	}
}

void URecallPhysicsSubsystem::ResetMotionType(const FRecallPhysicsBodyHandle& Handle)
{
	FScopeLock Lock(&DataGuard);
	if (FJPRPhysicsBodyRef* BodyRef = BodyRefMap.Find(Handle))
	{
		if (BodyRef->MotionTypeOverride.IsValid())
		{
			SetMotionType(Handle, BodyRef->Params.MotionType);
		}
	}
}
