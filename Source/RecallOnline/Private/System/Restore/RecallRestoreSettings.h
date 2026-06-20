// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"

#include "RecallRestoreSettings.generated.h"

/**
 * Settings for the Recall Restore.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Restore"))
class RECALLONLINE_API URecallRestoreSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	URecallRestoreSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UPROPERTY(EditAnywhere, config, Category=Restore)
	TSubclassOf<UUserWidget> RestoreWidgetClass;
	
	UPROPERTY(EditAnywhere, config, Category=Restore, meta=(ClampMin=1))
	int32 RestoreSpeed = 10;
	
	UPROPERTY(EditAnywhere, config, Category=Restore, meta=(ClampMin=10, ClampMax=100000))
	int32 BunchByteCount = 200;
};
