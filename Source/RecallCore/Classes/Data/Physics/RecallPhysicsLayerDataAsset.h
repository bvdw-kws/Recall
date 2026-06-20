// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Engine/DataAsset.h"

#include "RecallPhysicsLayerDataAsset.generated.h"

USTRUCT()
struct RECALLCORE_API FRecallPhysicsCollisionPreset
{
	GENERATED_BODY()

public:
	// Broadphase layer used by this layer
	UPROPERTY(EditAnywhere, meta=(GetOptions="GetBroadPhaseLayerNames"))
	FName BroadPhaseLayer = NAME_None;

	// Collision between this layer and other layers
	UPROPERTY(EditAnywhere, EditFixedSize)
	TMap<FName, bool> LayerCollisionPresets;

	// Collision between this layer and broadphase layers
	UPROPERTY(EditAnywhere, EditFixedSize)
	TMap<FName, bool> BroadPhaseCollisionPresets;
};

UCLASS()
class RECALLCORE_API URecallPhysicsLayerDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	URecallPhysicsLayerDataAsset();

	// UObject implementation Begin
public:
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	// UObject implementation Begin

public:
	static int32 GetLayerIndex(const FDataTableRowHandle& Handle, int32 DefaultValue = 0);

public:
	bool ShouldUseLandscapeCollision() const;
	float GetLandscapeFriction() const;
	
	bool ShouldUseWorldStaticMeshCollision() const;
	float GetWorldStaticMeshFriction() const;

	FName GetObjectLayer(int32 ObjectLayer) const;
	const TArray<FName>& GetObjectLayers() const;
	bool CanObjectCollide(int32 ObjectLayer1, int32 ObjectLayer2) const;
	bool CanObjectCollideWithBroadphase(int32 ObjectLayer1, int32 BroadphaseLayer2) const;
	bool CanObjectCollideWithBroadphase(const FName& ObjectLayer, const FName& BroadphaseLayer) const;

	FName GetObjectBroadPhaseLayer(int32 ObjectLayer) const;
	FName GetObjectBroadPhaseLayer(const FName& ObjectLayerName) const;
	int32 GetObjectBroadPhaseLayerIndex(int32 ObjectLayer, int32 DefaultValue = 0) const;

	FName GetBroadPhaseLayer(int32 BroadPhaseLayer) const;
	int32 GetBroadPhaseLayerCount() const;

	UFUNCTION(CallInEditor, Category=Editor)
	void UpdateLayers();

	const FName& GetBroadPhaseLayerName(int32 BroadphaseLayer) const;
	FDataTableRowHandle GetStaticLayerHandle() const;

protected:
	UPROPERTY(EditAnywhere)
	bool bUseLandscapeCollision = true;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition=bUseLandscapeCollision, EditConditionHides))
	float LandscapeFriction = 0.4f;
	
	UPROPERTY(EditAnywhere)
	bool bUseWorldStaticMeshCollision = true;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition=bUseWorldStaticMeshCollision, EditConditionHides))
	float WorldStaticMeshFriction = 0.4f;

	UPROPERTY(EditAnywhere, meta=(RowType="/Script/RecallCore.RecallPhysicsLayerTableRow"))
	TObjectPtr<UDataTable> LayerTable;

	// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
	// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
	// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
	// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
	// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
	UPROPERTY(EditAnywhere)
	TArray<FName> BroadPhaseLayers;

	UPROPERTY(EditAnywhere, EditFixedSize, meta=(ShowOnlyInnerProperties))
	TMap<FName, FRecallPhysicsCollisionPreset> CollisionPresets;

private:
	UFUNCTION()
	const TArray<FName>& GetBroadPhaseLayerNames() const;

	UPROPERTY(Transient)
	TArray<FName> LayerNames;

	void UpdateBroadphaseLayers();
};
