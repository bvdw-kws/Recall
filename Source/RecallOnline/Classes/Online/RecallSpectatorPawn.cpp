// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSpectatorPawn.h"

#include "Components/Pawn/RecallGameSimViewComponent.h"

ARecallSpectatorPawn::ARecallSpectatorPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAddDefaultMovementBindings = false;
	
	GameSimViewComponent = ObjectInitializer.CreateDefaultSubobject<URecallGameSimViewComponent>(
		this, TEXT("GameSimViewComponent"));
}

void ARecallSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();
}

void ARecallSpectatorPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARecallSpectatorPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	const TWeakObjectPtr<AActor> GameSimViewTarget = IsValid(GameSimViewComponent) ? GameSimViewComponent->GetGameSimViewTarget() : nullptr;
	if (GameSimViewTarget.IsValid())
	{
		GameSimViewTarget->CalcCamera(DeltaTime, OutResult);
	}
	else
	{		
		Super::CalcCamera(DeltaTime, OutResult);
	}
}

void ARecallSpectatorPawn::ViewPrevPlayer()
{
	ChangeViewPlayer(false);
}

void ARecallSpectatorPawn::ViewNextPlayer()
{
	ChangeViewPlayer(true);
}

void ARecallSpectatorPawn::ChangeViewPlayer(bool bNext) const
{
	const TArray<FString>& GameSimControllerIDs = GameSimViewComponent->GetGameSimControllerIDs();
	FString GameSimViewTargetControllerID = GameSimViewComponent->GetGameSimViewTargetControllerID();
	
	if (GameSimControllerIDs.Num() > 0)
	{
		auto GetNextGameSimControllerIndex = [&GameSimControllerIDs](bool bNext, int32 CurrentGameSimControllerIndex)
		{
			if (bNext)
			{
				return CurrentGameSimControllerIndex + 1 < GameSimControllerIDs.Num() ? CurrentGameSimControllerIndex + 1 : 0;
			}
			else
			{
				return CurrentGameSimControllerIndex > 0 ? CurrentGameSimControllerIndex - 1 : GameSimControllerIDs.Num() - 1;
			}
		};
		
		const int32 CurrentGameSimControllerIndex = GameSimControllerIDs.IndexOfByKey(GameSimViewTargetControllerID);
		const int32 NextGameSimControllerIndex = GetNextGameSimControllerIndex(bNext, CurrentGameSimControllerIndex);

		GameSimViewTargetControllerID = GameSimControllerIDs[NextGameSimControllerIndex];
	}
	else
	{
		GameSimViewTargetControllerID = FString();
	}
	
	GameSimViewComponent->SetGameSimViewTargetControllerID(GameSimViewTargetControllerID);
}
