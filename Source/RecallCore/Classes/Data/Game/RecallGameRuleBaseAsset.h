// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Engine/DataAsset.h"

#include "RecallGameRuleBaseAsset.generated.h"

struct FInstancedStruct;

/**
 * Base asset that can be expanded to define rules of the game.
 */
UCLASS(abstract, Blueprintable)
class RECALLCORE_API URecallGameRuleBaseAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FName GetPlayerStart(const UObject* WorldContextObject, const FString& PlayerID, const FInstancedStruct& CustomParameters) const { return NAME_None; }
	virtual bool IsBlockInputForMatchState(const FName& MatchState) const { return false; }
	
public:
	static const FPrimaryAssetType AssetType;
	
	// UObject implementation Begin
protected:	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	// UObject implementation End
};
