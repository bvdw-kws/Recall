// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallEntityComponent.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "MassEntityTraitBase.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "VisualLogger/VisualLogger.h"

URecallEntityComponent::URecallEntityComponent()
	: EntityConfig(*this)
{
	bAutoRegister = true;
	SetIsReplicatedByDefault(true);
}

void URecallEntityComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	ApplyEntityAssetName();
#endif // WITH_EDITOR
}

void URecallEntityComponent::PostEditImport()
{
	Super::PostEditImport();

#if WITH_EDITOR
	ApplyEntityAssetName();
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void URecallEntityComponent::ApplyEntityAssetName()
{
	AActor* OwnerActor = GetOwner();
	
	if (!IsValid(OwnerActor) || OwnerActor == OwnerActor->GetClass()->GetDefaultObject())
	{
		return;
	}
	
	const FString NewEntityAssetName = OwnerActor->GetActorLabel();
	if (EntityAssetName != NewEntityAssetName)
	{
		EntityAssetName = NewEntityAssetName;
		EntityConfig.SetOwner(*this);
		Modify();
	}
}

void URecallEntityComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITORONLY_DATA
	OnEditTraits.Broadcast();
#endif // WITH_EDITORONLY_DATA
}

void URecallEntityComponent::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		EntityConfig.SetOwner(*this);		
	}
}
#endif // WITH_EDITOR

bool URecallEntityComponent::ShouldSpawnInEmptyLevel() const
{
#if WITH_EDITORONLY_DATA
	return bEmptyLevelSpawn;
#else // WITH_EDITORONLY_DATA
	return false;
#endif
}

void URecallEntityComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	EntityConfig.SetOwner(*this);
}

void URecallEntityComponent::OnRegister()
{
	Super::OnRegister();

	if (IsRunningCommandlet() || IsRunningCookCommandlet() || GIsCookerLoadingPackage)
	{
		// ignore, we're not doing any registration while cooking or running a commandlet
		return;
	}

	if (GetOuter() == nullptr || GetOuter()->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || HasAnyFlags(RF_ArchetypeObject))
	{
		// we won't try registering a CDO's component with Mass
		ensure(false && "temp, wanna know this happened");
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr
#if WITH_EDITOR
		|| World->IsPreviewWorld()
		|| World->IsGameWorld() == false
#endif // WITH_EDITOR
		)
	{
		// we don't care about preview worlds. Those are transient, temporary worlds like the one created when opening a BP editor.
		return;
	}

	RegisterWithEntitySubsystem();
}

void URecallEntityComponent::OnUnregister()
{
	UE_VLOG(GetOwner(), LogTemp, Verbose, TEXT("%hs"), __FUNCTION__);

	UnregisterWithEntitySubsystem();

	Super::OnUnregister();
}

void URecallEntityComponent::RegisterWithEntitySubsystem()
{
	const UWorld* World = GetOwningWorld();
	URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World);
	UE_CVLOG_UELOG(EntitySystem == nullptr, GetOwner(), LogTemp, Error, TEXT("Unable to find URecallEntitySubsystem instance. Make sure the world is initialized"));
	if (ensureMsgf(EntitySystem, TEXT("Unable to find URecallEntitySubsystem instance. Make sure the world is initialized")))
	{
		EntitySystem->RegisterEntityComponent(this);
	}
}

void URecallEntityComponent::UnregisterWithEntitySubsystem()
{
	const UWorld* World = GetOwningWorld();
	if (URecallEntitySubsystem* EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(World))
	{
		UE_VLOG(GetOwner(), LogTemp, Verbose, TEXT("%hs"), __FUNCTION__);
		EntitySystem->ShutdownEntityComponent(this);
	}
}

UWorld* URecallEntityComponent::GetOwningWorld() const
{
	const AActor* Owner = GetOwner();
	const ULevel* Level = Owner ? Owner->GetLevel() : nullptr;
	return Level ? Level->OwningWorld : nullptr;
}

const UMassEntityTraitBase* URecallEntityComponent::GetTrait(const TSubclassOf<UMassEntityTraitBase> InClass) const
{
	// Helper function to find a trait from within an entity config asset
	auto FindTraitInConfig = []
	(const FMassEntityConfig& InEntityConfig, const TSubclassOf<UMassEntityTraitBase> InTraitClass)
	{
		const UMassEntityTraitBase* const* SearchFoundTrait = InEntityConfig.GetTraits().FindByPredicate([&InTraitClass](const UMassEntityTraitBase* InTrait)
		{
			return InTrait && InTrait->IsA(InTraitClass);
		});

		return SearchFoundTrait;
	};

	// First look through the component's traits
	const UMassEntityTraitBase* const* FoundTrait = FindTraitInConfig(EntityConfig, InClass);

	// If the trait was not found, then look through its parent entity configs, if there are any
	if(!FoundTrait)
	{
		const UMassEntityConfigAsset* ParentConfig = EntityConfig.GetParent();
		while(ParentConfig)
		{
			FoundTrait = FindTraitInConfig(ParentConfig->GetConfig(), InClass);
			
			if(FoundTrait)
			{
				break;
			}

			// Get next parent
			ParentConfig = ParentConfig->GetConfig().GetParent();
		}
	}

	return FoundTrait ? *FoundTrait : nullptr;
}

void URecallEntityComponent::SetEntityConfigParentAsset(UMassEntityConfigAsset* InParent)
{
	if (InParent)
	{
		EntityConfig.SetParentAsset(*InParent);
	}
}
