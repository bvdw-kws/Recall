// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallHUD_InGame.h"

#include "CommonUserWidget.h"
#include "ExtendedPrimaryGameLayoutTypes.h"
#include "Engine/AssetManager.h"
#include "RecallFrontendUtils.h"
#include "PrimaryGameLayout.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "Utility/Gameplay/RecallGameplayStatics.h"

void ARecallHUD_InGame::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);
}

void ARecallHUD_InGame::DrawHUD()
{
	Super::DrawHUD();

	const bool bIsSimulationPlaying = SimulationController && 
		SimulationController->HasSimulationStarted() && !SimulationController->IsSimulationPaused();
	
	if (bIsSimulationPlaying || URecallGameplayStatics::IsPlayingReplay(this))
	{
		FlushDebugStrings(GetWorld());
	}
}

void ARecallHUD_InGame::BeginSpectating()
{
	PushHUDWidget(SpectatingHUDWidgetClass);
}

void ARecallHUD_InGame::EndSpectating()
{
	RemoveHUDWidget();
}

void ARecallHUD_InGame::BeginPlaying()
{
	PushHUDWidget(PlayingHUDWidgetClass);
}

void ARecallHUD_InGame::EndPlaying()
{
	RemoveHUDWidget();
}

void ARecallHUD_InGame::OnPlayerDefeat(const FString& Reason)
{
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayerController()))
	{
		RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(
			TAG_UI_LAYER_GAMEMENU, true, DefeatWidgetClass);
	}
}

void ARecallHUD_InGame::OnGameEnd_Implementation(const FString& Reason)
{
}

void ARecallHUD_InGame::PushHUDWidget(const TSoftClassPtr<class UCommonUserWidget>& HUDWidgetClass)
{
	if (HUDWidgetClass.IsNull())
	{
		return;
	}
	
	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
	HUDWidgetStreamingHandle = StreamableManager.RequestAsyncLoad(HUDWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(this, [this, HUDWidgetClass]()
		{			
			HUDWidget = CreateWidget<UCommonUserWidget>(GetOwningPlayerController(), HUDWidgetClass.Get());
			if (HUDWidget.IsValid())
			{
				HUDWidget->AddToPlayerScreen();
			}
			HUDWidgetStreamingHandle.Reset();
		})
	);
}

void ARecallHUD_InGame::RemoveHUDWidget()
{
	if (HUDWidgetStreamingHandle.IsValid())
	{
		HUDWidgetStreamingHandle->ReleaseHandle();
		HUDWidgetStreamingHandle.Reset();
	}
	
	if (HUDWidget.IsValid())
	{
		HUDWidget->RemoveFromParent();
		HUDWidget.Reset();
	}
}
