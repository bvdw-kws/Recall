// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "RecallSnapshotTypes.h"

#include "RecallMultiSimSnapshotTypes.generated.h"

USTRUCT()
struct RECALLSNAPSHOT_API FRecallMultiSimSnapshot
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint32 Frame{ 0 };

	UPROPERTY()
	double Time{ 0.0 };

	UPROPERTY()
	TArray<FRecallSimulationSnapshot> SimSnapshots;
};
