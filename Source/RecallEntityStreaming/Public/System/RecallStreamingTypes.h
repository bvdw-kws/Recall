// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "HierarchicalHashGrid2D.h"

class URecallEntityComponent;

/**
 * Item stored in the streaming hash grid for efficient spatial queries
 */
struct RECALLENTITYSTREAMING_API FRecallStreamingGridItem
{
	FRecallStreamingGridItem() 
		: Location(FVector::ZeroVector)
		, StreamingDistance(0.0f)
		, Priority(0)
	{
	}
	
	bool operator==(const FRecallStreamingGridItem& Other) const
	{
		return Component == Other.Component;
	}

	/** The entity component to potentially stream */
	TWeakObjectPtr<URecallEntityComponent> Component;
	
	/** Cached position from the component's actor */
	FVector Location;
	
	/** Custom streaming distance override */
	float StreamingDistance;
	
	/** Streaming priority (higher = loaded first) */
	int32 Priority;
	
	// Note: Asset name removed - we'll get it from the component when needed
};

/** 
 * Spatial hash grid for efficient proximity queries of streamable entities
 * Uses 2 levels of hierarchy with 4x ratio between levels for different query scales
 */
typedef THierarchicalHashGrid2D<4, 4, FRecallStreamingGridItem> FRecallStreamingHashGrid2D;