// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallWorldTransitionTypes.generated.h"

UENUM()
enum class ERecallWorldTransitionEventType : uint8
{
	ENTER,
	LEAVE,
};

/*
* Field transition event, triggered each time a player enters or leave this field.
*/
USTRUCT()
struct RECALLSIMULATION_API FRecallWorldTransitionEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint32 Frame = 0;

	UPROPERTY(VisibleAnywhere)
	FString PlayerId;

	UPROPERTY(VisibleAnywhere)
	ERecallWorldTransitionEventType Type = ERecallWorldTransitionEventType::ENTER;

	bool IsEnterEvent() const { return Type == ERecallWorldTransitionEventType::ENTER; }
	bool IsLeaveEvent() const { return Type == ERecallWorldTransitionEventType::LEAVE; }

};

USTRUCT()
struct RECALLSIMULATION_API FRecallWorldTransitionData
{
	GENERATED_BODY()

	/* List of player field transition events. */
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallWorldTransitionEvent> Events;
};
