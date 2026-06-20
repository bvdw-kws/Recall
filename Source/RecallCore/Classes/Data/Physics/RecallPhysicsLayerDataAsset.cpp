// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsLayerDataAsset.h"

#include "Data/Physics/RecallPhysicsLayerTableRow.h"

URecallPhysicsLayerDataAsset::URecallPhysicsLayerDataAsset()
	: Super()
{
	BroadPhaseLayers.Add(TEXT("Static"));
	BroadPhaseLayers.Add(TEXT("Dynamic"));
}

void URecallPhysicsLayerDataAsset::PostLoad()
{
	Super::PostLoad();

	UpdateLayers();
	UpdateBroadphaseLayers();
}

#if WITH_EDITOR
void URecallPhysicsLayerDataAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == TEXT("LayerTable"))
	{
		UpdateLayers();
	}
	else if (PropertyChangedEvent.GetPropertyName() == TEXT("BroadPhaseLayers"))
	{
		UpdateBroadphaseLayers();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

bool URecallPhysicsLayerDataAsset::ShouldUseLandscapeCollision() const
{
	return bUseLandscapeCollision;
}

bool URecallPhysicsLayerDataAsset::ShouldUseWorldStaticMeshCollision() const
{
	return bUseWorldStaticMeshCollision;
}

float URecallPhysicsLayerDataAsset::GetWorldStaticMeshFriction() const
{
	return WorldStaticMeshFriction;
}

FName URecallPhysicsLayerDataAsset::GetObjectLayer(int32 ObjectLayer) const
{
	const TArray<FName>& Layers = GetObjectLayers();
	if (!Layers.IsValidIndex(ObjectLayer))
	{
		return NAME_None;
	}

	return Layers[ObjectLayer];
}

float URecallPhysicsLayerDataAsset::GetLandscapeFriction() const
{
	return LandscapeFriction;
}

const TArray<FName>& URecallPhysicsLayerDataAsset::GetObjectLayers() const
{
	return LayerNames;
}

int32 URecallPhysicsLayerDataAsset::GetLayerIndex(const FDataTableRowHandle& Handle, int32 DefaultValue /*= 0*/)
{
	ensureAlwaysMsgf(!Handle.IsNull(), TEXT("%hs Layer is null"), __FUNCTION__);
	
	if (Handle.DataTable && ensure(Handle.DataTable->RowStruct == FRecallPhysicsLayerTableRow::StaticStruct()))
	{
		const FName& Layer = Handle.RowName;
		const TArray<FName> Layers = Handle.DataTable->GetRowNames();
		const int32 Value = Layers.IndexOfByKey(Layer);

		if (ensureMsgf(Value != INDEX_NONE,
			TEXT("%hs Could not find physics layer: %s"), __FUNCTION__, *Layer.ToString()))
		{
			return Value;
		}
	}
	
	return DefaultValue;
}

bool URecallPhysicsLayerDataAsset::CanObjectCollide(int32 ObjectLayer1, int32 ObjectLayer2) const
{
	const TArray<FName>& Layers = GetObjectLayers();
	if (!Layers.IsValidIndex(ObjectLayer1) || !Layers.IsValidIndex(ObjectLayer2))
	{
		return false;
	}

	const FName& LayerName1 = Layers[ObjectLayer1];
	const FName& LayerName2 = Layers[ObjectLayer2];
	const FRecallPhysicsCollisionPreset* CollisionPreset1 = CollisionPresets.Find(LayerName1);
	const FRecallPhysicsCollisionPreset* CollisionPreset2 = CollisionPresets.Find(LayerName2);
	if (CollisionPreset1 == nullptr || CollisionPreset2 == nullptr)
	{
		return false;
	}
	
	const bool* bCollide1 = CollisionPreset1->LayerCollisionPresets.Find(LayerName2);
	const bool* bCollide2 = CollisionPreset2->LayerCollisionPresets.Find(LayerName1);
	if (bCollide1 == nullptr || bCollide2 == nullptr)
	{
		return false;
	}
	
	// Both objects must collide with each other or collision may become unpredictable.
	return *bCollide1 && *bCollide2;
}

bool URecallPhysicsLayerDataAsset::CanObjectCollideWithBroadphase(int32 ObjectLayer1, int32 BroadphaseLayer2) const
{
	const FName LayerName1 = GetObjectLayer(ObjectLayer1);
	const FName BroadPhaseLayer2Name = GetBroadPhaseLayer(BroadphaseLayer2);

	return CanObjectCollideWithBroadphase(LayerName1, BroadPhaseLayer2Name);
}

bool URecallPhysicsLayerDataAsset::CanObjectCollideWithBroadphase(const FName& ObjectLayer,
	const FName& BroadphaseLayer) const
{
	const FRecallPhysicsCollisionPreset* CollisionPreset = CollisionPresets.Find(ObjectLayer);
	if (CollisionPreset == nullptr)
	{
		return false;
	}
	
	const bool* BroadPhaseCollisionPresetPtr = CollisionPreset->BroadPhaseCollisionPresets.Find(BroadphaseLayer);
	if (BroadPhaseCollisionPresetPtr == nullptr)
	{
		return false;
	}

	return *BroadPhaseCollisionPresetPtr;
}

const TArray<FName>& URecallPhysicsLayerDataAsset::GetBroadPhaseLayerNames() const
{
	return BroadPhaseLayers;
}

void URecallPhysicsLayerDataAsset::UpdateBroadphaseLayers()
{
	for (TPair<FName, FRecallPhysicsCollisionPreset>& CollisionPreset : CollisionPresets)
	{
		TArray<FName> OldBroadphaseLayers;
		CollisionPreset.Value.BroadPhaseCollisionPresets.GenerateKeyArray(OldBroadphaseLayers);

		// Remove deprecated broadphase layers
		for (const FName& OldBroadphaseLayer : OldBroadphaseLayers)
		{
			if (!BroadPhaseLayers.Contains(OldBroadphaseLayer))
			{
				CollisionPreset.Value.BroadPhaseCollisionPresets.Remove(OldBroadphaseLayer);
			}
		}

		// Add new broadphase layers
		for (const FName& BroadphaseLayer : BroadPhaseLayers)
		{
			if (!CollisionPreset.Value.BroadPhaseCollisionPresets.Contains(BroadphaseLayer))
			{
				CollisionPreset.Value.BroadPhaseCollisionPresets.Add(BroadphaseLayer, false);
			}
		}
	}
}

void URecallPhysicsLayerDataAsset::UpdateLayers()
{
	LayerNames = IsValid(LayerTable) ? LayerTable->GetRowNames() : TArray<FName>();

	TArray<FName> OldLayers;
	CollisionPresets.GenerateKeyArray(OldLayers);

	// Remove deprecated layers
	for (const FName& OldLayer : OldLayers)
	{
		if (!LayerNames.Contains(OldLayer))
		{
			CollisionPresets.Remove(OldLayer);
		}
	}

	// Add new layers
	for (const FName& Layer : LayerNames)
	{
		if (!CollisionPresets.Contains(Layer))
		{
			FRecallPhysicsCollisionPreset& CollisionPreset = CollisionPresets.Add(Layer);
			if (BroadPhaseLayers.IsValidIndex(0))
			{
				CollisionPreset.BroadPhaseLayer = BroadPhaseLayers[0];
			}
			
			for (const FName& BroadPhaseLayer : BroadPhaseLayers)
			{
				CollisionPreset.BroadPhaseCollisionPresets.Add(BroadPhaseLayer, true);
			}
		}
	}

	for (TPair<FName, FRecallPhysicsCollisionPreset>& CollisionPreset : CollisionPresets)
	{
		TArray<FName> OldPresetLayers;
		CollisionPreset.Value.LayerCollisionPresets.GenerateKeyArray(OldPresetLayers);

		// Remove deprecated layers
		for (const FName& OldPresetLayer : OldPresetLayers)
		{
			if (!LayerNames.Contains(OldPresetLayer))
			{
				CollisionPreset.Value.LayerCollisionPresets.Remove(OldPresetLayer);
			}
		}

		// Add new layers
		for (const FName& Layer : LayerNames)
		{
			if (!CollisionPreset.Value.LayerCollisionPresets.Contains(Layer))
			{
				CollisionPreset.Value.LayerCollisionPresets.Add(Layer, false);
			}
		}
	}
}

FName URecallPhysicsLayerDataAsset::GetObjectBroadPhaseLayer(int32 ObjectLayer) const
{
	const TArray<FName>& Layers = GetObjectLayers();

	if (!Layers.IsValidIndex(ObjectLayer))
	{
		return NAME_None;
	}

	const FName& LayerName = Layers[ObjectLayer];
	return GetObjectBroadPhaseLayer(LayerName);
}

FName URecallPhysicsLayerDataAsset::GetObjectBroadPhaseLayer(const FName& ObjectLayerName) const
{
	const FRecallPhysicsCollisionPreset* CollisionPreset = CollisionPresets.Find(ObjectLayerName);
	if (CollisionPreset == nullptr)
	{
		return NAME_None;
	}
	return CollisionPreset->BroadPhaseLayer;
}

int32 URecallPhysicsLayerDataAsset::GetObjectBroadPhaseLayerIndex(int32 ObjectLayer, int32 DefaultValue /*= 0*/) const
{
	const FName BroadPhaseLayerName = GetObjectBroadPhaseLayer(ObjectLayer);
	const int32 BroadPhaseLayerIndex = BroadPhaseLayers.IndexOfByKey(BroadPhaseLayerName);
	if (BroadPhaseLayerIndex == INDEX_NONE)
	{
		return DefaultValue;
	}

	return BroadPhaseLayerIndex;
}

FName URecallPhysicsLayerDataAsset::GetBroadPhaseLayer(int32 BroadPhaseLayer) const
{
	if (BroadPhaseLayers.IsValidIndex(BroadPhaseLayer))
	{
		return BroadPhaseLayers[BroadPhaseLayer];
	}
	return NAME_None;
}

int32 URecallPhysicsLayerDataAsset::GetBroadPhaseLayerCount() const
{
	return BroadPhaseLayers.Num();
}

const FName& URecallPhysicsLayerDataAsset::GetBroadPhaseLayerName(int32 BroadphaseLayer) const
{
	check(BroadPhaseLayers.IsValidIndex((BroadphaseLayer)));
	return BroadPhaseLayers[BroadphaseLayer];
}

FDataTableRowHandle URecallPhysicsLayerDataAsset::GetStaticLayerHandle() const
{
	FDataTableRowHandle Handle;
	Handle.DataTable = LayerTable;
	Handle.RowName = TEXT("Static");
	
	return Handle;
}
