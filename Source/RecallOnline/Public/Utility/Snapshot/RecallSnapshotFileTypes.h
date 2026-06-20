// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallSnapshotFileTypes.generated.h"

/*
* Snapshot containing our serialized game sim.
*/
USTRUCT()
struct RECALLONLINE_API FRecallSnapshot
{
	GENERATED_BODY()

	/* Map the snapshot was taken from */
	UPROPERTY(VisibleAnywhere)
	FString MapName;

	/* Time stamp of our snapshot (UTC) */
	UPROPERTY(VisibleAnywhere)
	FDateTime UtcTimeStamp;

	/* Serialized data of our game simulation */
	UPROPERTY(VisibleAnywhere)
	TArray<uint8> SnapshotMemory;

	/* This snapshot was saved while playing with editor only data */
	UPROPERTY(VisibleAnywhere)
	bool bIsEditorOnlyData = false;
};
