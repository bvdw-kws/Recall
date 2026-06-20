// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Player/RecallPlayerStartSubsystem.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

#if WITH_EDITOR
#include "Engine/PlayerStartPIE.h"
#endif // WITH_EDITOR

URecallPlayerStartSubsystem::URecallPlayerStartSubsystem()
	:Super()
{
}

void URecallPlayerStartSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URecallPlayerStartSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallPlayerStartSubsystem::Start(const FRecallSimulationStartParams& Params)
{
#if WITH_EDITORONLY_DATA
	PlayerStartPIE = nullptr;
#endif // WITH_EDITORONLY_DATA
	
	PlayerStarts.Reset();

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

#if WITH_EDITOR
		if (PlayerStart->IsA<APlayerStartPIE>())
		{
#if WITH_EDITORONLY_DATA
			PlayerStartPIE = PlayerStart;
#endif // WITH_EDITORONLY_DATA
		}
		else
#endif // WITH_EDITOR
		{
			PlayerStarts.Add(PlayerStart);
		}
	}
}

bool URecallPlayerStartSubsystem::FindPlayerStart(const FRandomStream& RandomStream,
	FVector& OutLocation, FQuat& OutRotation, FName PlayerStartTag /*= NAME_None*/, bool bAllowStartPIE /*= true*/) const
{
	const FTransform FoundPlayerStart = FindDefaultPlayerStart(RandomStream, PlayerStartTag, bAllowStartPIE);

	OutLocation = FoundPlayerStart.GetLocation();
	OutRotation = FoundPlayerStart.GetRotation();
	return true;
}

FTransform URecallPlayerStartSubsystem::FindDefaultPlayerStart(const FRandomStream& RandomStream,
	const FName& PlayerStartTag, bool bAllowStartPIE) const
{
	// This method is based on AGameModeBase::FindPlayerStart_Implementation
#if WITH_EDITOR
#if WITH_EDITORONLY_DATA
	// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
	if (bAllowStartPIE && PlayerStartPIE)
	{
		return PlayerStartPIE->GetActorTransform();
	}
#endif // WITH_EDITORONLY_DATA
#endif // WITH_EDITOR

	if (!PlayerStartTag.IsNone())
	{		
		const TObjectPtr<APlayerStart>* FoundPlayerStart = PlayerStarts.FindByPredicate([&PlayerStartTag](const TObjectPtr<APlayerStart> PS)
		{
			return PS && PS->PlayerStartTag == PlayerStartTag;
		});
		if (FoundPlayerStart == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("%hs Could not find APlayerStart for: %s"),
				__FUNCTION__, *PlayerStartTag.ToString());
		}
		else if (*FoundPlayerStart)
		{
			return (*FoundPlayerStart)->GetActorTransform();			
		}
	}
	
	APlayerStart* DefaultPlayerStart = nullptr;

	if (PlayerStarts.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%hs No APlayerStart was found"), __FUNCTION__);
	}
	else
	{
		DefaultPlayerStart = PlayerStarts[RandomStream.RandRange(0, PlayerStarts.Num() - 1)];
	}

	return IsValid(DefaultPlayerStart) ? DefaultPlayerStart->GetActorTransform() : FTransform::Identity;
}
