// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Gameplay/RecallGameplayStatics.h"

#include "Camera/CameraComponent.h"
#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/GameState/RecallReplayGameComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Online/RecallGameMode.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerController.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

ARecallPlayerController* URecallGameplayStatics::GetRecallPlayerController(const UObject* WorldContextObject, int32 PlayerIndex)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return Cast<ARecallPlayerController>(UGameplayStatics::GetPlayerController(MainWorld, PlayerIndex));
}

ARecallGameMode* URecallGameplayStatics::GetRecallGameMode(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(MainWorld));
}

ARecallGameState_InGame* URecallGameplayStatics::GetRecallInGameState(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(MainWorld));
}

APlayerCameraManager* URecallGameplayStatics::GetRecallPlayerCameraManager(const UObject* WorldContextObject, int32 PlayerIndex)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return UGameplayStatics::GetPlayerCameraManager(MainWorld, PlayerIndex);
}

int32 URecallGameplayStatics::GetNumPlayerJoinedGame(const UObject* WorldContextObject)
{
	int32 NumPlayerJoinedGame = 0;

	if (WorldContextObject)
	{
		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			for (FConstControllerIterator Iterator = World->GetControllerIterator(); Iterator; ++Iterator)
			{
				if (const ARecallPlayerController* PlController = Cast<ARecallPlayerController>(Iterator->Get()))
				{
					if (IsValid(PlController) && PlController->HasJoinedGame())
					{
						NumPlayerJoinedGame++;
					}
				}
				else
				{					
					NumPlayerJoinedGame++;
				}
			}
		}
	}

	return NumPlayerJoinedGame;
}

int32 URecallGameplayStatics::GetNumLocalPlayerJoinedGame(const UObject* WorldContextObject)
{
	return GetLocalPlayerJoinedGame(WorldContextObject).Num();
}

TArray<ARecallPlayerController*> URecallGameplayStatics::GetLocalPlayerJoinedGame(const UObject* WorldContextObject)
{
	TArray<ARecallPlayerController*> LocalPlayerJoinedGameArray;

	if (WorldContextObject)
	{
		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				if (ARecallPlayerController* PlController = Cast<ARecallPlayerController>(Iterator->Get()))
				{
					if (IsValid(PlController) && PlController->IsLocalPlayerController() && PlController->HasJoinedGame())
					{
						LocalPlayerJoinedGameArray.Add(PlController);
					}
				}
			}
		}
	}

	return LocalPlayerJoinedGameArray;
}

bool URecallGameplayStatics::IsPlayingReplay(const UObject* WorldContextObject)
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject));
	if (IsValid(GameState))
	{
		const URecallReplayGameComponent* ReplayComponent = GameState->GetReplayComponentChecked();
		return ReplayComponent->IsPlayingReplay();
	}

	return false;
}

bool URecallGameplayStatics::IsRestoringGameSimulation(const UObject* WorldContextObject)
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject));
	if (IsValid(GameState))
	{
		const URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();
		return ClientRestoreComponent->IsRestoringGameSimulation();
	}

	return false;
}

void URecallGameplayStatics::GetFormattedTime(const UObject* WorldContextObject, double TimeSeconds, int32& NumHours,
	int32& NumMinutes, int32& NumSeconds)
{
	// Get whole minutes
	NumHours = FMath::FloorToInt(TimeSeconds/ (60.f * 60.f));
	NumMinutes = FMath::FloorToInt(TimeSeconds/60.f);
	// Get seconds not part of whole minutes
	NumSeconds = FMath::FloorToInt(TimeSeconds-(NumMinutes*60.f));
}

void URecallGameplayStatics::ProjectWorldToScreen(APlayerController const* InPlayer, const FVector& InWorldPosition,
	FVector2D& OutScreenPosition)
{
	ULocalPlayer* const LP = InPlayer ? InPlayer->GetLocalPlayer() : nullptr;
	if (LP && LP->ViewportClient)
	{
		FSceneViewProjectionData NewProjectionData;
		if (LP->GetProjectionData(LP->ViewportClient->Viewport, NewProjectionData))
		{
			const FMatrix ViewProjectionMatrix = NewProjectionData.ComputeViewProjectionMatrix();
			const FIntRect ViewRectangle = NewProjectionData.GetConstrainedViewRect();
			const FPlane Result = ViewProjectionMatrix.TransformFVector4(FVector4(InWorldPosition, 1.f));

			// the result of this will be x and y coords in -1..1 projection space
			const float RHW = 1.0f / FMath::Abs(Result.W);
			const FPlane PosInScreenSpace = FPlane(Result.X * RHW, Result.Y * RHW, Result.Z * RHW, Result.W);

			// Move from projection space to normalized 0..1 UI space
			const float NormalizedX = ( PosInScreenSpace.X / 2.f ) + 0.5f;
			const float NormalizedY = 1.f - ( PosInScreenSpace.Y / (2.f * FMath::Max(FMath::Abs(NormalizedX), 1.f)) ) - 0.5f;

			const FVector2D RayStartViewRectSpace(
				( NormalizedX * static_cast<float>(ViewRectangle.Width()) ),
				( NormalizedY * static_cast<float>(ViewRectangle.Height()) )
				);

			OutScreenPosition = RayStartViewRectSpace + FVector2D(static_cast<float>(ViewRectangle.Min.X), static_cast<float>(ViewRectangle.Min.Y));



		}
	}
}

void URecallGameplayStatics::ReturnToMainMenu(const UObject* WorldContextObject)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		GameInstance->ReturnToMainMenu();
	}
}

void URecallGameplayStatics::RestartLevel(const UObject* WorldContextObject)
{
	if (AGameMode* GameMode = Cast<AGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		GameMode->RestartGame();
	}
}

bool URecallGameplayStatics::ProjectWorldToSceneCaptureScreenPosition(USceneCaptureComponent2D* SceneCaptureComponent2D, const FVector& WorldPosition, FVector2D& OutScreenPosition)
{
	if (SceneCaptureComponent2D && SceneCaptureComponent2D->TextureTarget)
	{
		TOptional<FMatrix> CustomProjectionMatrix;
		if (SceneCaptureComponent2D->bUseCustomProjectionMatrix)
		{
			CustomProjectionMatrix = SceneCaptureComponent2D->CustomProjectionMatrix;
		}

		FMinimalViewInfo ViewInfo;
		SceneCaptureComponent2D->GetCameraView(0.0f, ViewInfo);

		FMatrix ViewMatrix = FMatrix::Identity;
		FMatrix ProjectionMatrix = FMatrix::Identity;
		FMatrix ViewProjectionMatrix = FMatrix::Identity;
		UGameplayStatics::CalculateViewProjectionMatricesFromMinimalView(ViewInfo, CustomProjectionMatrix, ViewMatrix, ProjectionMatrix, ViewProjectionMatrix);

		const FIntPoint TargetSize(
			SceneCaptureComponent2D->TextureTarget->SizeX,
			SceneCaptureComponent2D->TextureTarget->SizeY);
		const FIntRect ViewRect(FIntPoint(0, 0), TargetSize);

		return FSceneView::ProjectWorldToScreen(WorldPosition, ViewRect, ViewProjectionMatrix, OutScreenPosition);
	}

	OutScreenPosition = FVector2d::Zero();
	return false;
}

bool URecallGameplayStatics::ProjectWorldToCameraScreenPosition(UCameraComponent* CameraComponent, const FVector& WorldPosition, FVector2D& OutScreenPosition)
{
	FIntRect ViewRect(0, 0);
	if (CameraComponent && FindCameraViewRect(CameraComponent, ViewRect))
	{
		TOptional<FMatrix> CustomProjectionMatrix;

		FMinimalViewInfo ViewInfo;
		CameraComponent->GetCameraView(0.0f, ViewInfo);

		FMatrix ViewMatrix = FMatrix::Identity;
		FMatrix ProjectionMatrix = FMatrix::Identity;
		FMatrix ViewProjectionMatrix = FMatrix::Identity;
		UGameplayStatics::CalculateViewProjectionMatricesFromMinimalView(ViewInfo, CustomProjectionMatrix, ViewMatrix, ProjectionMatrix, ViewProjectionMatrix);

		return FSceneView::ProjectWorldToScreen(WorldPosition, ViewRect, ViewProjectionMatrix, OutScreenPosition);
	}

	OutScreenPosition = FVector2d::Zero();
	return false;
}

void URecallGameplayStatics::DeprojectCameraScreenPositionToWorld(UCameraComponent* CameraComponent, float ScreenX, float ScreenY, FVector& OutWorldPosition, FVector& OutWorldDirection)
{
	FIntRect ViewRect(0, 0);
	if (CameraComponent && FindCameraViewRect(CameraComponent, ViewRect))
	{
		TOptional<FMatrix> CustomProjectionMatrix;

		FMinimalViewInfo ViewInfo;
		CameraComponent->GetCameraView(0.0f, ViewInfo);

		FMatrix ViewMatrix = FMatrix::Identity;
		FMatrix ProjectionMatrix = FMatrix::Identity;
		FMatrix ViewProjectionMatrix = FMatrix::Identity;
		UGameplayStatics::CalculateViewProjectionMatricesFromMinimalView(ViewInfo, CustomProjectionMatrix, ViewMatrix, ProjectionMatrix, ViewProjectionMatrix);

		const FMatrix InvViewProjectionMatrix = ViewProjectionMatrix.Inverse();
		FSceneView::DeprojectScreenToWorld(FVector2D(ScreenX, ScreenY), ViewRect, InvViewProjectionMatrix, OutWorldPosition, OutWorldDirection);
	}
	else
	{
		OutWorldPosition = FVector::ZeroVector;
		OutWorldDirection = FVector::ZeroVector;
	}
}

bool URecallGameplayStatics::FindCameraViewRect(class UCameraComponent* CameraComponent, FIntRect& OutViewRect)
{
	if (CameraComponent)
	{
		const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(CameraComponent);
		const UGameViewportClient* GameViewportClient = GameInstance ? GameInstance->GetGameViewportClient() : NULL;

		if (GameViewportClient && GameViewportClient->Viewport)
		{
			const FIntPoint SizeXY = GameViewportClient->Viewport->GetSizeXY();

			if (CameraComponent->bConstrainAspectRatio)
			{
				OutViewRect = GameViewportClient->Viewport->CalculateViewExtents(CameraComponent->AspectRatio, FIntRect(FIntPoint(0, 0), SizeXY));
			}
			else
			{
				OutViewRect = FIntRect(FIntPoint(0, 0), SizeXY);
			}
			return true;
		}
	}

	OutViewRect = FIntRect(0, 0);
	return false;
}
