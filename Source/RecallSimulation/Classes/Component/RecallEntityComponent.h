// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/ActorComponent.h"
#include "MassEntityConfigAsset.h"

#include "RecallEntityComponent.generated.h"

/**
 */
UCLASS(Blueprintable, ClassGroup=Mass, meta=(BlueprintSpawnableComponent), hidecategories=(Sockets, Collision))
class RECALLSIMULATION_API URecallEntityComponent : public UActorComponent
{
	GENERATED_BODY()

	URecallEntityComponent();

public:
	/**
	 * Entity config of the entity to spawn.
	 */
	const FMassEntityConfig& GetEntityConfig() const { return EntityConfig; }
	FMassEntityConfig& GetMutableEntityConfig() { return EntityConfig; }

	UFUNCTION(BlueprintCallable)
	void SetEntityConfigParentAsset(UMassEntityConfigAsset* InParent);

	/**
	 * Unique world name of the entity.
	 */
	const FString& GetEntityAssetName() const { return EntityAssetName; }

	/**
	 * Does this entity has to be rollback?
	 */
	FORCEINLINE bool IsMutableEntity() const { return bMutableEntity; }
	void SetMutableEntity(bool bMutable) { bMutableEntity = bMutable; }

	/**
	 * Is streaming enabled for this entity?
	 */
	FORCEINLINE bool IsStreamingEnabled() const { return bEnableStreaming; }

	/**
	 * When testing in an empty level, should this entity be spawned?
	 * Entities such as static colliders should always be spawned.
	 */
	bool ShouldSpawnInEmptyLevel() const;

#if WITH_EDITOR
	/**
	 * Update the unique world name of the entity.
	 */
	void ApplyEntityAssetName();
#endif // WITH_EDITOR

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEntityComponentEditTraitsSignature);
#if WITH_EDITORONLY_DATA
public:
	/**
	 * Editor callback every time a trait is edited.
	 */
	UPROPERTY(BlueprintAssignable)
	FEntityComponentEditTraitsSignature OnEditTraits;

protected:
	UPROPERTY(EditAnywhere)
	bool bEmptyLevelSpawn{ false };
#endif // WITH_EDITORONLY_DATA

	/**
	 * Does this entity have to be rollback?
	 */
	UPROPERTY(EditAnywhere)
	bool bMutableEntity{ true };

	/** Enable streaming for this entity (disabled by default for safety) */
	UPROPERTY(EditAnywhere, Category="Streaming")
	bool bEnableStreaming = false;

	/** Distance from view target at which entity should be loaded */
	UPROPERTY(EditAnywhere, Category="Streaming", meta=(EditCondition="bEnableStreaming"))
	float StreamingDistance = 5000.0f;

	/** Priority for streaming (higher = loaded first) */
	UPROPERTY(EditAnywhere, Category="Streaming", meta=(EditCondition="bEnableStreaming"))
	int32 StreamingPriority = 0;

	/** Override visibility streaming for this entity */
	UPROPERTY(EditAnywhere, Category="Streaming", meta=(EditCondition="bEnableStreaming"))
	bool bOverrideVisibilityStreaming = false;

	/** Custom visibility settings when override is enabled */
	UPROPERTY(EditAnywhere, Category="Streaming", meta=(EditCondition="bEnableStreaming && bOverrideVisibilityStreaming"))
	bool bIgnoreVisibility = false;
	
public:
	/**
	 * Looks for the first instance of a specified EntityTrait class from within this component's EntityConfigAsset
	 * 
	 * @param InClass The entity trait class to look for from within this component's EntityConfigAsset
	 * @return Returns the trait class, if found, else return nullptr
	 */
	UFUNCTION(BlueprintPure)
	const UMassEntityTraitBase* GetTrait(const TSubclassOf<UMassEntityTraitBase> InClass) const;

	template<typename T=UMassEntityTraitBase>
	const T* GetTrait() const
	{
		return Cast<T>(GetTrait(T::StaticClass()));
	}
	
	/** Get streaming distance for this entity */
	FORCEINLINE float GetStreamingDistance() const { return StreamingDistance; }
	
	/** Get streaming priority for this entity */
	FORCEINLINE int32 GetStreamingPriority() const { return StreamingPriority; }

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnComponentCreated() override;

	virtual void PostLoad() override;
	virtual void PostEditImport() override;

#if WITH_EDITOR
protected:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
#endif // WITH_EDITOR
	
protected:

	/**
	 * Internal asset name used by the simulation to reference actors.
	 */
	UPROPERTY(VisibleAnywhere, DisplayName=AssetName)
	FString EntityAssetName;

	UPROPERTY(EditAnywhere)
	FMassEntityConfig EntityConfig;
	
	void RegisterWithEntitySubsystem();
	void UnregisterWithEntitySubsystem();

	UWorld* GetOwningWorld() const;
	
};
