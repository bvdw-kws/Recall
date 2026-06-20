// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisualLogger/VisualLogger.h"
#include "StructView.h"

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallTrait, Log, All);

struct FMassEntityConfig;

namespace Recall::Trait::Utils
{

template<typename T = AActor>
FORCEINLINE extern T* AsActor(UObject& Owner)
{
	AActor* Actor = nullptr;
	if (AActor* AsActor = Cast<AActor>(&Owner))
	{
		Actor = AsActor;
	}

	UE_CVLOG_UELOG(Actor == nullptr, &Owner, LogRecallTrait, Error, TEXT("Trying to extract %s from %s failed")
		, *T::StaticClass()->GetName(), *Owner.GetName());

	return Actor;
}

template<typename T = UActorComponent>
FORCEINLINE extern T* AsComponent(UObject& Owner)
{
	T* Component = nullptr;
	if (AActor* AsActor = Cast<AActor>(&Owner))
	{
		Component = AsActor->FindComponentByClass<T>();
	}
	else
	{
		Component = Cast<T>(&Owner);
	}

	UE_CVLOG_UELOG(Component == nullptr, &Owner, LogRecallTrait, Error, TEXT("Trying to extract %s from %s failed")
		, *T::StaticClass()->GetName(), *Owner.GetName());

	return Component;
}

RECALLSIMULATION_API extern uint32 GetStructCrc32(const UScriptStruct& ScriptStruct, const uint8* StructMemory, const uint32 CRC = 0);
RECALLSIMULATION_API extern uint32 GetStructCrc32(const FConstStructView& StructView, const uint32 CRC = 0);

template<typename T>
FORCEINLINE extern uint32 GetFragmentCrc32(const T& Fragment)
{
	return GetStructCrc32(FConstStructView::Make(Fragment));
}

RECALLSIMULATION_API extern FMassEntityConfig* GetMutableEntityConfig(UObject* Outer);
RECALLSIMULATION_API extern const FMassEntityConfig* GetEntityConfig(const UObject* Outer);

} // namespace Recall::Trait::Utils

#define MS_FRAGMENT_CRC_32(Fragment) Recall::Trait::Utils::GetFragmentCrc32(Fragment)
