// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallSnapshotInterface.generated.h"

UINTERFACE()
class RECALLCORE_API URecallSnapshotInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallSnapshotInterface
{
	GENERATED_IINTERFACE_BODY()

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLoadSnapshot, uint32 /*Frame*/, double /*TimeSeconds*/, bool /*bIsRollback*/);

public:
	virtual void SaveQuick() = 0;
	virtual void LoadQuick() = 0;

	virtual bool TakeSnapshot(TArray<uint8>& OutSnapshot) const = 0;
	virtual bool LoadSnapshot(const TArray<uint8>& Snapshot) = 0;

	virtual FOnLoadSnapshot& GetOnLoadSnapshotEvent() = 0;

};
