// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Serialization/BitReader.h"
#include "Serialization/BitWriter.h"
#include "System/Input/RecallInputQueueTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallInputMouseWorldPositionTest, "Recall.Input.MouseWorldPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecallInputMouseWorldPositionTest::RunTest(const FString& Parameters)
{
	const FVector WorldPosition(1234.0, -5678.0, 90.0);

	FRecallInput Input;
	Input.MousePosition = FVector2D(640.0, 360.0);
	Input.MouseWorldPosition = WorldPosition;

	// Inputs that only differ by their world position must not be considered equal,
	// otherwise the input queue merges them and the cursor never moves.
	FRecallInput OtherInput = Input;
	TestTrue(TEXT("Identical inputs are equal"), Input == OtherInput);
	OtherInput.MouseWorldPosition = WorldPosition + FVector(100.0, 0.0, 0.0);
	TestTrue(TEXT("Inputs differing only by world position are different"), Input != OtherInput);

	// Full serialization round trip
	{
		FBitWriter Writer(1024 * 8, true);
		bool bSuccess = false;
		FRecallInput ToWrite = Input;
		ToWrite.NetSerialize(Writer, nullptr, bSuccess);
		TestTrue(TEXT("NetSerialize write succeeded"), bSuccess);

		FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
		FRecallInput Read;
		Read.NetSerialize(Reader, nullptr, bSuccess);
		TestTrue(TEXT("NetSerialize read succeeded"), bSuccess);
		TestEqual(TEXT("NetSerialize keeps the world position"), Read.MouseWorldPosition, FVector(WorldPosition));
		TestTrue(TEXT("NetSerialize keeps the input intact"), Read == Input);
	}

	// Delta serialization when only the world position changed
	{
		FRecallInput Previous = Input;
		Previous.MouseWorldPosition = FVector(-10.0, 20.0, 0.0);

		FBitWriter Writer(1024 * 8, true);
		bool bSuccess = false;
		FRecallInput ToWrite = Input;
		ToWrite.NetSerializeFromPrevious(Previous, Writer, nullptr, bSuccess);

		FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
		FRecallInput Read;
		Read.NetSerializeFromPrevious(Previous, Reader, nullptr, bSuccess);
		TestEqual(TEXT("NetSerializeFromPrevious applies the new world position"), Read.MouseWorldPosition, FVector(WorldPosition));
		TestTrue(TEXT("NetSerializeFromPrevious keeps the input intact"), Read == Input);
	}

	// Delta serialization when nothing changed keeps the previous world position
	{
		FBitWriter Writer(1024 * 8, true);
		bool bSuccess = false;
		FRecallInput ToWrite = Input;
		ToWrite.NetSerializeFromPrevious(Input, Writer, nullptr, bSuccess);

		FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
		FRecallInput Read;
		Read.NetSerializeFromPrevious(Input, Reader, nullptr, bSuccess);
		TestEqual(TEXT("Unchanged world position is kept from the previous input"), Read.MouseWorldPosition, FVector(WorldPosition));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
