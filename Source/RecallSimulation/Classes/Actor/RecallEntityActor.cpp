// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallEntityActor.h"

#include "Component/RecallEntityComponent.h"

ARecallEntityActor::ARecallEntityActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent"));
	EntityComponent = ObjectInitializer.CreateDefaultSubobject<URecallEntityComponent>(this, TEXT("EntityComponent"));
}

void ARecallEntityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (World == nullptr ||
		(!World->IsPreviewWorld() &&
			(World->IsEditorWorld() == false || World->IsGameWorld())
			)
		)
	{
		return;
	}

	if (EntityComponent)
	{
		EntityComponent->ApplyEntityAssetName();
#if WITH_EDITORONLY_DATA
		EntityComponent->OnEditTraits.AddUniqueDynamic(this, &ThisClass::OnEditTraits);
#endif // WITH_EDITORONLY_DATA
	}

	OnEditTraits();
#endif // WITH_EDITOR
}

void ARecallEntityActor::BeginPlay()
{
	Super::BeginPlay();

	if (bHideActorInGameOnBeginPlay)
	{
#if WITH_EDITOR
		if (const UWorld* World = GetWorld())
		{
			const bool bShowEntityActor = (World->IsEditorWorld() && !World->IsPlayInEditor()) || World->IsPreviewWorld();

			SetActorHiddenInGame(!bShowEntityActor);
		}
		else
		{
			SetActorHiddenInGame(true);
		}
#else // WITH_EDITOR
		// Entity actor should only be used as a container for our entity config
		SetActorHiddenInGame(true);
#endif
	}
}

TObjectPtr<URecallEntityComponent> ARecallEntityActor::GetEntityComponent() const
{
	return EntityComponent;
}

FName ARecallEntityActor::GetEntityAssetName() const
{
	if (EntityComponent)
	{
		return *EntityComponent->GetEntityAssetName();
	}
	return NAME_None;
}

void ARecallEntityActor::OnEditTraits_Implementation()
{
	// Do nothing by default, just here to allow override in native classes
}
