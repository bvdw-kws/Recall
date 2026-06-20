// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallInputActionTableRow.h"

static bool ValidateTable(const UDataTable* DataTable)
{
	return IsValid(DataTable) && DataTable->RowStruct && DataTable->RowStruct == FRecallInputActionTableRow::StaticStruct();
}

TArray<FName> FRecallInputActionTableRow::GetInputActionNames(const UDataTable* DataTable)
{
	if (!ValidateTable(DataTable))
	{
		return {};
	}

	return DataTable->GetRowNames();
}

int32 FRecallInputActionTableRow::GetInputActionIndex(const FDataTableRowHandle& RowHandle)
{
	return GetInputActionIndex(RowHandle.DataTable, RowHandle.RowName);
}

int32 FRecallInputActionTableRow::GetInputActionIndex(const UDataTable* DataTable, const FName& RowName)
{
	return GetInputActionNames(DataTable).IndexOfByKey(RowName);
}

const UInputAction* FRecallInputActionTableRow::GetInputAction(const UDataTable* DataTable, const FName& RowName)
{
	if (IsValid(DataTable))
	{
		static const FString ContextString(TEXT("FRecallInputActionTableRow::GetInputAction"));
		if (const FRecallInputActionTableRow* Row = DataTable->FindRow<FRecallInputActionTableRow>(RowName, ContextString))
		{
			return Row->InputAction;
		}
	}

	return NULL;
}

ERecallControllerInputCommand FRecallInputActionTableRow::GetControllerInputCommandByAction(const FDataTableRowHandle& RowHandle)
{
	const int32 Index = GetInputActionIndex(RowHandle);

	if (FMath::IsWithin(Index, 0, MAX_RECALL_INPUT_COUNT))
	{
		return static_cast<ERecallControllerInputCommand>(1 << Index);
	}
	else
	{
		return ERecallControllerInputCommand::None;
	}
}
