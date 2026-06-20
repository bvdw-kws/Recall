// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RecallSimulationInsightTypes.h"

#include "RecallSimulationInsightInterface.generated.h"

UINTERFACE()
class RECALLCORE_API URecallSimulationInsightInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallSimulationInsightInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/* Oldest frame we want to keep track of. */
	virtual void SetReportFromFrame(uint32 Frame) = 0;

	/* Generate a frame report from the StartFrame to the EndFrame included. */
	virtual FRecallSimulationInsight GenerateReportInRange(uint32 StartFrame, uint32 EndFrame) const = 0;
};
