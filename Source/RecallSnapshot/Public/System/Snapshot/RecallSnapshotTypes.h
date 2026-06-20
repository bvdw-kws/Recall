// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallSnapshotTypes.generated.h"

RECALLSNAPSHOT_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallSnapshot, Log, All);

USTRUCT()
struct RECALLSNAPSHOT_API FRecallSimulationSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Frame { 0 };

	UPROPERTY()
	double TimeSeconds { 0.0 };

	UPROPERTY()
	TArray<FInstancedStruct> DataSnapshots;
};
