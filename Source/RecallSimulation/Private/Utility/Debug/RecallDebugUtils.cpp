// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "Utility/Debug/RecallDebugUtils.h"

#include "System/Representation/RecallRepresentationEventSubsystem.h"

namespace Recall::Debug::Utils
{

static void PushEvent(const UWorld* InWorld, FRecallRepresentationEventFunc Callback)
{
	if (URecallRepresentationEventSubsystem* RepresentationEventSystem = UWorld::GetSubsystem<URecallRepresentationEventSubsystem>(InWorld))
	{
		RepresentationEventSystem->PushEvent(Callback);
	}
}
	
void DrawDebugSphere(const UWorld* InWorld, FVector const& Center, float Radius, int32 Segments, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	PushEvent(InWorld, [InWorld, Center, Radius, Segments, Color, bPersistentLines, LifeTime, DepthPriority, Thickness]()
	{
		::DrawDebugSphere(InWorld, Center, Radius, Segments, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	});
}
	
void DrawDebugLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	PushEvent(InWorld, [InWorld, LineStart, LineEnd, Color, bPersistentLines, LifeTime, DepthPriority, Thickness]()
	{
		::DrawDebugLine(InWorld, LineStart, LineEnd, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	});
}
	
void DrawDebugString(const UWorld* InWorld, FVector const& TextLocation, const FString& Text, class AActor* TestBaseActor, FColor const& TextColor, float Duration, bool bDrawShadow, float FontScale)
{
	PushEvent(InWorld, [InWorld, TextLocation, Text, TestBaseActor, TextColor, Duration, bDrawShadow, FontScale]()
	{
		::DrawDebugString(InWorld, TextLocation, Text, TestBaseActor, TextColor, Duration, bDrawShadow, FontScale);
	});
}
	
}