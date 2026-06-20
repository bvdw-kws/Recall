// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"

namespace Recall::Input::Utils
{

FORCEINLINE void AddIntOption(const FString& Key, FString& Options, int32 Value)
{
	Options += FString::Printf(TEXT("?%s=%d"), *Key, Value);
}

FORCEINLINE void AddOption(const FString& Key, FString& Options, const FString& Value)
{
	Options += FString::Printf(TEXT("?%s=%s"), *Key, *Value);
}

FORCEINLINE void AddNameOption(const FString& Key, FString& Options, const FName& Value)
{
	Options += FString::Printf(TEXT("?%s=%s"), *Key, *Value.ToString());
}

FORCEINLINE void AddVectorOption(const FString& Key, FString& Options, const FVector& Value)
{
	Options += FString::Printf(TEXT("?%s=%s"), *Key, *Value.ToString());
}

FORCEINLINE FString GetOption(const FString& Options, const FString& Key)
{
	return UGameplayStatics::ParseOption(Options, Key);
}

FORCEINLINE FName GetNameOption(const FString& Options, const FString& Key)
{
	return *UGameplayStatics::ParseOption(Options, Key);
}

FORCEINLINE FVector GetVectorOption(const FString& Options, const FString& Key)
{
	const FString Option = UGameplayStatics::ParseOption(Options, Key);
	FVector Result = FVector::ZeroVector;
	bool bIsValid = false;
	UKismetStringLibrary::Conv_StringToVector(Option, Result, bIsValid);
	ensureAlwaysMsgf(bIsValid, TEXT("Failed to convert"));
	return Result;
}

FORCEINLINE int32 GetIntOption(const FString& Options, const FString& Key, int32 DefaultValue = 0)
{
	return UGameplayStatics::GetIntOption(Options, Key, DefaultValue);
}

template<typename EnumType>
FORCEINLINE void AddEnumOption(const FString& Key, FString& Options, EnumType Value)
{
	AddIntOption(Key, Options, static_cast<int32>(Value));
}

template<typename EnumType>
FORCEINLINE EnumType GetEnumOption(const FString& Options, const FString& Key, EnumType DefaultValue)
{
	return static_cast<EnumType>(GetIntOption(Options, Key, static_cast<int32>(DefaultValue)));
}

} // namespace Recall::Input::Utils
