// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "System/Input/RecallInputQueueTypes.h"

#include "RecallInputActionTableRow.generated.h"

USTRUCT(BlueprintType)
struct RECALLCORE_API FRecallInputActionTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UInputAction> InputAction;

public:
	static TArray<FName> GetInputActionNames(const UDataTable* DataTable);
	static int32 GetInputActionIndex(const FDataTableRowHandle& RowHandle);
	static int32 GetInputActionIndex(const UDataTable* DataTable, const FName& RowName);
	static const UInputAction* GetInputAction(const UDataTable* DataTable, const FName& RowName);
	static ERecallControllerInputCommand GetControllerInputCommandByAction(const FDataTableRowHandle& RowHandle);

};
