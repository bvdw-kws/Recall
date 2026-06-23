// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "Physics/RecallPhysicsBodyHandle.h"
#include "Physics/RecallPhysicsConstrainHandle.h"
#include "Physics/RecallPhysicsHitEvent.h"
#include "Physics/RecallPhysicsTypes.h"
#include "Physics/RecallPhysicsShapeTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Mass/EntityHandle.h"

#include "RecallPhysicsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRecallPhysics, Log, All);

#if WITH_JOLT_PHYSICS
namespace JPH
{
	class TempAllocator;
	class PhysicsSystem;
	class BodyInterface;
	class Body;
	class BodyID;
	class JobSystemThreadPool;
	class BroadPhaseLayerInterface;
	class StateRecorder;

	class BroadPhaseLayer;
	using ObjectLayer = uint16;
	class ObjectVsBroadPhaseLayerFilter;
	class ObjectLayerPairFilter;
	class StateRecorderFilter;
} // namespace JPH

class FRecallBodyActivationListener;
class FRecallContactListener;
#endif // WITH_JOLT_PHYSICS

class URecallPhysicsObjectFactory;
class FRecallPhysicsBody;
struct FRecallPhysicsBodySnapshot;

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	template<typename T=FRecallPhysicsShape>
	FRecallPhysicsBodyHandle CreateShape(const FMassEntityHandle& Entity, const T& Shape, const FRecallPhysicsBodyParameters& Params)
	{
		// Make sure that we do not call this outside of the simulation.
		CheckSimulationProcessingPhase();
		
		FRecallPhysicsBodyHandle Handle;
		CreateShape_Internal(Entity, FInstancedStruct::Make<T>(Shape), Shape.FactoryClass, Params, Handle);
		return Handle;
	}

	template<typename T=FRecallPhysicsShape>
	void CreateStaticShape(const T& Shape, const FVector& Location, const FQuat& Rotation, float Friction)
	{
		CreateStaticShape_Internal(FInstancedStruct::Make<T>(Shape), Location, Rotation, Shape.FactoryClass, Friction);
	}

	template<typename T=FRecallPhysicsShape>
	FRecallPhysicsBodyHandle CreateDynamicStaticShape(const T& Shape, const FVector& Location, const FQuat& Rotation, float Friction)
	{
		return CreateDynamicStaticShape_Internal(FInstancedStruct::Make<T>(Shape), Location, Rotation, Shape.FactoryClass, Friction);
	}

	void CreateFixedConstrain(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2);
	void RemoveAllConstrains(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2);

	void ReleaseBody(const FRecallPhysicsBodyHandle& Handle);
	TWeakPtr<FRecallPhysicsBody> GetMutableBody(const FRecallPhysicsBodyHandle& Handle);
	TWeakPtr<const FRecallPhysicsBody> GetBody(const FRecallPhysicsBodyHandle& Handle) const;

	template<typename T>
	TWeakPtr<T> GetMutableBody(const FRecallPhysicsBodyHandle& Handle)
	{
		return StaticCastWeakPtr<T>(GetMutableBody(Handle));
	}
	
	template<typename T>
	TWeakPtr<const T> GetBody(const FRecallPhysicsBodyHandle& Handle)
	{
		return StaticCastWeakPtr<T>(GetMutableBody(Handle));
	}

	void SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, uint16 Layer);
	void ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle);
	bool HasLayerOverride(const FRecallPhysicsBodyHandle& Handle) const;

	void SetMotionType(const FRecallPhysicsBodyHandle& Handle, ERecallPhysicsMotionType MotionType, ERecallPhysicsActivation ActivationMode = ERecallPhysicsActivation::Activate);
	void ResetMotionType(const FRecallPhysicsBodyHandle& Handle);

	void GenerateHitEvents(const TSet<FRecallPhysicsBodyHandle>& GeneratesHitEventsBodyHandles);
	TArray<FRecallPhysicsHitEvent> GetHitEvents(const FRecallPhysicsBodyHandle& Handle) const;
	bool HasHitEvent(const FRecallPhysicsBodyHandle& Handle) const;
	void ResetHitEvents();

	int32 GetNumContactEvents() const;

	TWeakObjectPtr<const class URecallPhysicsLayerDataAsset> GetPhysicsLayer() const;
	bool ShouldRestorePhysicsBody(const uint32 BodyID) const;

	/**
	 * Casts for hits from physics body's handle's center-of-mass in the Direction specified
	 * 
	 * @param Handle Handle to the physics body to cast from
	 * @param Direction (Recall Units) Direction and length of the cast (anything beyond this length will not be reported as a hit)
	 * @param OutHits Output of hit bodies
	 * @return Returns true if anything was hit
	 */
	// bool CastShape(const FRecallPhysicsBodyHandle& Handle, const FIntVector& Direction, TArray<FRecallPhysicsCastShapeResult>& OutHits) const;

public:
	virtual void TickPhysics(float DeltaTime);

protected:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// UWorldSubsystem implementation Begin

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Start(const FRecallSimulationStartParams& Params) override;
	virtual int32 GetStartOrderPriority() const override { return Recall::SimReactSystem::StartOrder::HighPriority; }
	virtual void Reset() override;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override;
	// IRecallSimulationReactSystemInterface implementation End

protected:
	virtual void ReleasePhysicsObjects();
	virtual bool ShouldGenerateHitEvent(const uint32 BodyID) const;

	URecallPhysicsObjectFactory* CreateShapeFactory(const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass);
	
	void CreateStaticShape_Internal(const FInstancedStruct& Shape, const FVector& Location, const FQuat& Rotation,
		const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass, float Friction);

	FRecallPhysicsBodyHandle CreateDynamicStaticShape_Internal(const FInstancedStruct& Shape, const FVector& Location,
		const FQuat& Rotation, const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass, float Friction);

	void CreateShape_Internal(const FMassEntityHandle& Entity, const FInstancedStruct& Shape,
		const TSubclassOf<URecallPhysicsObjectFactory>& FactoryClass, const FRecallPhysicsBodyParameters& Params,
		FRecallPhysicsBodyHandle& Handle);
	void CheckSimulationProcessingPhase() const;
	
protected:
	// Our physics bodies's serial number generator.
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 SerialNumberGenerator{ 0 };

protected:
#if WITH_JOLT_PHYSICS
	TSharedPtr<JPH::TempAllocator> temp_allocator;
	TSharedPtr<JPH::JobSystemThreadPool> job_system;
	TSharedPtr<JPH::PhysicsSystem> physics_system;
	TSharedPtr<JPH::BroadPhaseLayerInterface> broad_phase_layer_interface;
	TSharedPtr<JPH::ObjectVsBroadPhaseLayerFilter> object_vs_broadphase_layer_filter;
	TSharedPtr<JPH::ObjectLayerPairFilter> object_layer_pair_filter;
	TSharedPtr<JPH::StateRecorderFilter> state_recorder_filter;

	TSharedPtr<FRecallBodyActivationListener> body_activation_listener;
	TSharedPtr<FRecallContactListener> contact_listener;
#endif // WITH_JOLT_PHYSICS
	
	struct FRecallPhysicsBodyRef
	{
		FMassEntityHandle Entity;
		TSharedPtr<FRecallPhysicsBody> Body;
		FInstancedStruct Shape;
		FRecallPhysicsBodyParameters Params;
		bool bRestoreBody = true;
		TUniquePtr<uint16> LayerOverride;
		TUniquePtr<ERecallPhysicsMotionType> MotionTypeOverride;
	};
	TMap<FRecallPhysicsBodyHandle, FRecallPhysicsBodyRef> BodyRefMap;
	TMap<uint32, FRecallPhysicsBodyHandle> BodyHandleMap;

	TSet<FRecallPhysicsConstrainHandle> ConstrainRefs;

	void AddConstrainRef_Internal(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2);
	void RemoveConstrainRef_Internal(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2);

	TMap<FRecallPhysicsBodyHandle, TArray<FRecallPhysicsHitEvent>> HitEvents;

protected:
	UPROPERTY(Transient)
	TObjectPtr<const class URecallPhysicsLayerDataAsset> PhysicsLayer;
	UPROPERTY(Transient)
	TWeakObjectPtr<class ALandscape> LandscapeActor;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallEntitySubsystem> EntitySystem;

	mutable FCriticalSection DataGuard;
	mutable FCriticalSection HitEventGuard;

	template<typename ShapeType>
	FRecallPhysicsBodyHandle RegisterPhysicsBody(const FMassEntityHandle& Entity, const TSharedPtr<FRecallPhysicsBody>& Body, const ShapeType& Shape)
	{
		FRecallPhysicsBodyHandle Handle;
		RegisterPhysicsBody_Internal(Entity, Body, FInstancedStruct::Make<ShapeType>(Shape), Handle);
		return Handle;
	}

	void ReleaseBody_Internal(const FRecallPhysicsBodyHandle& Handle, bool bCleanUp = false);

	void CreatePhysicsSystem();
	void DeletePhysicsSystem();
	
	void OnActorsInitialized(const FActorsInitializedParams& Params);

	FRecallPhysicsBodySnapshot TakeBodySnapshot(const FRecallPhysicsBodyHandle& Handle) const;
	
	// void RunSample();

#if WITH_JOLT_PHYSICS
	friend class URecallPhysicsObjectFactory;
	
	JPH::PhysicsSystem& GetPhysicsSystem() const;
	JPH::BodyInterface& GetBodyInterface() const;

	void SaveState(JPH::StateRecorder& OutState);
	bool RestoreState(JPH::StateRecorder& InState);
#endif // WITH_JOLT_PHYSICS

};

template<>
struct TMassExternalSubsystemTraits<URecallPhysicsSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
