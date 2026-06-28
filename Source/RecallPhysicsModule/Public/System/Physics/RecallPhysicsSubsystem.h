// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "System/JPRPhysicsSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "Physics/RecallPhysicsBodyHandle.h"
#include "Physics/RecallPhysicsObjects.h"
#include "Physics/RecallPhysicsConstrainHandle.h"
#include "Physics/RecallPhysicsHitEvent.h"
#include "Physics/RecallPhysicsTypes.h"
#include "Physics/JPRPhysicsShapeTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Mass/EntityHandle.h"

#include "RecallPhysicsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRecallPhysics, Log, All);

class UJPRPhysicsObjectFactory;
struct FJPRPhysicsBodySnapshot;

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsSubsystem : public UJPRPhysicsSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	template<typename T=FJPRPhysicsShape>
	FJPRPhysicsBodyHandle CreateShape(const FMassEntityHandle& Entity, const T& Shape, const FJPRPhysicsBodyParameters& Params)
	{
		// Make sure that we do not call this outside of the simulation.
		CheckSimulationProcessingPhase();
		
		FJPRPhysicsBodyHandle Handle;
		CreateShape_Internal(Entity, FInstancedStruct::Make<T>(Shape), Shape.FactoryClass, Params, Handle);
		return Handle;
	}

	template<typename T=FJPRPhysicsShape>
	void CreateStaticShape(const T& Shape, const FVector& Location, const FQuat& Rotation, float Friction)
	{
		CreateStaticShape_Internal(FInstancedStruct::Make<T>(Shape), Location, Rotation, Shape.FactoryClass, Friction);
	}

	template<typename T=FJPRPhysicsShape>
	FJPRPhysicsBodyHandle CreateMutableStaticShape(const T& Shape, const FVector& Location, const FQuat& Rotation, float Friction)
	{
		return CreateMutableStaticShape_Internal(FInstancedStruct::Make<T>(Shape), Location, Rotation, Shape.FactoryClass, Friction);
	}

	void CreateFixedConstrain(const FJPRPhysicsBodyHandle& Handle1, const FJPRPhysicsBodyHandle& Handle2);
	void RemoveAllConstrains(const FJPRPhysicsBodyHandle& Handle1, const FJPRPhysicsBodyHandle& Handle2);

	void ReleaseBody(const FJPRPhysicsBodyHandle& Handle);
	FJPRPhysicsBodyView GetMutableBody(const FJPRPhysicsBodyHandle& Handle);
	FConstRecallPhysicsBodyView GetBody(const FJPRPhysicsBodyHandle& Handle) const;

	void SetLayerOverride(const FJPRPhysicsBodyHandle& Handle, uint16 Layer);
	void ClearLayerOverride(const FJPRPhysicsBodyHandle& Handle);
	bool HasLayerOverride(const FJPRPhysicsBodyHandle& Handle) const;

	void SetMotionType(const FJPRPhysicsBodyHandle& Handle, EJPRPhysicsMotionType MotionType, EJPRPhysicsActivation ActivationMode = EJPRPhysicsActivation::Activate);
	void ResetMotionType(const FJPRPhysicsBodyHandle& Handle);

	void GenerateHitEvents(const TSet<FJPRPhysicsBodyHandle>& GeneratesHitEventsBodyHandles);
	TArray<FRecallPhysicsHitEvent> GetHitEvents(const FJPRPhysicsBodyHandle& Handle) const;
	bool HasHitEvent(const FJPRPhysicsBodyHandle& Handle) const;
	void ResetHitEvents();

	int32 GetNumContactEvents() const;

	TWeakObjectPtr<const class UJPRPhysicsLayerDataAsset> GetPhysicsLayer() const;

public:
	virtual void TickPhysics(float DeltaTime);

public:
	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// USubsystem implementation End

	// FTickableGameObject implementation Begin
	virtual bool IsTickable() const override { return false; }
	// FTickableGameObject implementation End
	
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
	
	// UJPRPhysicsSubsystem implementation Begin
	virtual void SetBodyObjectLayer(uint32 BodyID, uint16 Layer) override;
	virtual EJPRPhysicsMotionType GetBodyMotionType(uint32 BodyID) const override;
	virtual void SetBodyMotionType(uint32 BodyID, EJPRPhysicsMotionType MotionType, EJPRPhysicsActivation ActivationMode) override;
	virtual bool CreateFixedConstraint(uint32 BodyID1, uint32 BodyID2, bool bActivate) override;
	virtual void RemoveFixedConstraints(uint32 BodyID1, uint32 BodyID2) override;
	virtual TArray<FJPRContactEvent> ConsumeContactEvents() override;
	virtual int32 GetNumPendingContactEvents() const override;
	virtual bool ShouldRestorePhysicsBody(const uint32 BodyID) const override;
	// UJPRPhysicsSubsystem implementation End

	void CreateStaticShape_Internal(const FInstancedStruct& Shape, const FVector& Location, const FQuat& Rotation,
		const TSubclassOf<UJPRPhysicsObjectFactory>& FactoryClass, float Friction);

	FJPRPhysicsBodyHandle CreateMutableStaticShape_Internal(const FInstancedStruct& Shape, const FVector& Location,
		const FQuat& Rotation, const TSubclassOf<UJPRPhysicsObjectFactory>& FactoryClass, float Friction);

	void CreateShape_Internal(const FMassEntityHandle& Entity, const FInstancedStruct& Shape,
		const TSubclassOf<UJPRPhysicsObjectFactory>& FactoryClass, const FJPRPhysicsBodyParameters& Params,
		FJPRPhysicsBodyHandle& Handle);
	void CheckSimulationProcessingPhase() const;
	
protected:
	// Our physics bodies's serial number generator.
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 SerialNumberGenerator{ 0 };

protected:
	struct FJPRPhysicsBodyRef
	{
		FMassEntityHandle Entity;
		TSharedPtr<FJPRPhysicsBody> Body;
		FInstancedStruct Shape;
		FJPRPhysicsBodyParameters Params;
		bool bRestoreBody = true;
		TUniquePtr<uint16> LayerOverride;
		TUniquePtr<EJPRPhysicsMotionType> MotionTypeOverride;
	};
	TMap<FJPRPhysicsBodyHandle, FJPRPhysicsBodyRef> BodyRefMap;
	TMap<uint32, FJPRPhysicsBodyHandle> BodyHandleMap;

	TSet<FRecallPhysicsConstrainHandle> ConstrainRefs;

	void AddConstrainRef_Internal(const FJPRPhysicsBodyHandle& Handle1, const FJPRPhysicsBodyHandle& Handle2);
	void RemoveConstrainRef_Internal(const FJPRPhysicsBodyHandle& Handle1, const FJPRPhysicsBodyHandle& Handle2);

	TMap<FJPRPhysicsBodyHandle, TArray<FRecallPhysicsHitEvent>> HitEvents;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<class ALandscape> LandscapeActor;
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallEntitySubsystem> EntitySystem;

	mutable FCriticalSection DataGuard;
	mutable FCriticalSection HitEventGuard;

	template<typename ShapeType>
	FJPRPhysicsBodyHandle RegisterPhysicsBody(const FMassEntityHandle& Entity, const TSharedPtr<FJPRPhysicsBody>& Body, const ShapeType& Shape)
	{
		FJPRPhysicsBodyHandle Handle;
		RegisterPhysicsBody_Internal(Entity, Body, FInstancedStruct::Make<ShapeType>(Shape), Handle);
		return Handle;
	}

	void ReleaseBody_Internal(const FJPRPhysicsBodyHandle& Handle, bool bCleanUp = false);

	void OnActorsInitialized(const FActorsInitializedParams& Params);

	FJPRPhysicsBodySnapshot TakeBodySnapshot(const FJPRPhysicsBodyHandle& Handle) const;

};

template<>
struct TMassExternalSubsystemTraits<URecallPhysicsSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
