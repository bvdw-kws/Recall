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
#include "RecallPhysicsSnapshotOps.h"
#include "System/Physics/RecallPhysicsSnapshot.h"
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

	// The serial number is the Jolt BodyID, whose index must stay below MaxBodies.
	Tracker.SetMaxSerialNumber(GetDefault<UJPRPhysicsSettings>()->MaxBodies - 1);

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
	ReleasePhysicsObjects();

	DeletePhysicsSystem();
	CreatePhysicsSystem();
	
	Tracker.ResetSerialNumberGenerator();
	Tracker.SetMaxSerialNumber(GetDefault<UJPRPhysicsSettings>()->MaxBodies - 1);
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

	FRecallPhysicsSnapshotOps Ops(*this);
	Tracker.Save(Ops, OutSnapshot.GetMutable<FRecallPhysicsSnapshot>());
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

	CheckPhysicsAccess();

	// Same guard as the create/release paths. It is recursive, CreateShape_Internal takes it again from inside Restore.
	FScopeLock Lock(&DataGuard);

	FRecallPhysicsSnapshotOps Ops(*this);
	Tracker.Restore(Ops, *InData);
}

void URecallPhysicsSubsystem::ReleasePhysicsObjects()
{
	CheckPhysicsAccess();
	
	FRecallPhysicsSnapshotOps Ops(*this);
	Tracker.ReleaseAll(Ops);
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
	if (const FRecallPhysicsBodyHandle* BodyHandlePtr = Tracker.FindHandle(BodyID))
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
	if (const FRecallPhysicsBodyHandle* BodyHandlePtr = Tracker.FindHandle(BodyID))
	{
		if (const FRecallPhysicsBodyRef* BodyRefPtr = Tracker.FindBodyRef(*BodyHandlePtr))
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
		const FRecallPhysicsBodyHandle* Body1HandlePtr = Tracker.FindHandle(Body1ID);

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

		if (const FRecallPhysicsBodyHandle* Body2HandlePtr = Tracker.FindHandle(Body2ID))
		{
			HitEvent.ColliderBodyHandle = *Body2HandlePtr;

			if (const FRecallPhysicsBodyRef* Body2Ref = Tracker.FindBodyRef(*Body2HandlePtr))
			{
				// Prevent the entity from detecting hit with itself
				const FRecallPhysicsBodyRef* Body1Ref = Tracker.FindBodyRef(*Body1HandlePtr);
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
		if (Hits.IsEmpty())
		{
			// Bodies without hits are the vast majority, logging them makes the desync log huge.
			continue;
		}

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
	const FRecallPhysicsBodyHandle Handle = Tracker.AllocateHandle();
	if (!ensureMsgf(Handle.IsValid(), TEXT("%hs No free physics body ID (MaxBodies=%u)"), __FUNCTION__, GetDefault<UJPRPhysicsSettings>()->MaxBodies))
	{
		return Handle;
	}

	const TSharedPtr<FJPRPhysicsBody> Body = CreateBodyFromShape(Handle.SerialNumber, Shape, FactoryClass, Params);
	if (Body.IsValid())
	{
		Body->SetPositionAndRotation(Location, Rotation);
		Body->Activate();
	}

	Tracker.RegisterBody(Handle, FRecallPhysicsBodyRef{ StaticDummyEntity, Body, Shape, Params });

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
		Handle = Tracker.AllocateHandle();
		if (!ensureMsgf(Handle.IsValid(), TEXT("%hs No free physics body ID (MaxBodies=%u)"), __FUNCTION__, GetDefault<UJPRPhysicsSettings>()->MaxBodies))
		{
			return;
		}
	}

	const TSharedPtr<FJPRPhysicsBody> Body = CreateBodyFromShape(Handle.SerialNumber, Shape, FactoryClass, Params);

	Tracker.RegisterBody(Handle, FRecallPhysicsBodyRef{ Entity, Body, Shape, Params, bRestoreBody });
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
	Tracker.AddConstrainRef(Handle1, Handle2);
}

void URecallPhysicsSubsystem::RemoveConstrainRef_Internal(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	Tracker.RemoveConstrainRef(Handle1, Handle2);
}

void URecallPhysicsSubsystem::ReleaseBody(FRecallPhysicsBodyHandle& Handle)
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);

	if (!Handle.IsValid()) return;

	FScopeLock Lock(&DataGuard);
	ReleaseBody_Internal(Handle);
	Handle = FRecallPhysicsBodyHandle::Invalid();
}

void URecallPhysicsSubsystem::ReleaseBody_Internal(const FRecallPhysicsBodyHandle& Handle, bool bCleanUp /*= false*/)
{
	CheckPhysicsAccess();

	FRecallPhysicsSnapshotOps Ops(*this);
	Tracker.ReleaseBody(Ops, Handle);
}

FRecallPhysicsBodyView URecallPhysicsSubsystem::GetMutableBody(const FRecallPhysicsBodyHandle& Handle)
{
	CheckPhysicsAccess();
	
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this);

	if (Handle.IsValid())
	{
		if (const FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
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
		if (const FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
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
	if (FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
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
	if (const FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
	{
		return BodyRef->LayerOverride.IsValid();
	}

	return false;
}

void URecallPhysicsSubsystem::ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle)
{
	CheckPhysicsAccess();
	
	FScopeLock Lock(&DataGuard);
	if (FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
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
	if (FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
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
	if (FRecallPhysicsBodyRef* BodyRef = Tracker.FindBodyRef(Handle))
	{
		if (BodyRef->MotionTypeOverride.IsValid())
		{
			SetMotionType(Handle, BodyRef->Params.MotionType);
		}
	}
}
