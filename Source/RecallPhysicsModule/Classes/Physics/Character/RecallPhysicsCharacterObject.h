// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/JPRPhysicsBody.h"
#include "RecallPhysicsCharacterTypes.h"

struct FRecallPhysicsCharacter;

#if WITH_JOLT_PHYSICS
namespace JPH
{
	class Character;
} // namespace JPH
#endif // WITH_JOLT_PHYSICS

/**
* Wrapper Object for JPH::Character.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsCharacterBody : public FJPRPhysicsBody
{
public:
	void InitCharacter(const FRecallPhysicsCharacter& CharacterShape,
		const FJPRPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);
	void PostSimulation();

	bool IsSupported() const;
	ERecallPhysicsCharacterGroundState GetGroundState() const;
	
public:
	virtual void Activate() override;
	virtual void Desactivate() override;
	
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

protected:
	virtual void ReleasePhysicsObject() override;

#if WITH_JOLT_PHYSICS
	virtual void SaveState(JPH::StateRecorder& State) override;
	virtual void RestoreState(JPH::StateRecorder& State) override;

private:
	TSharedPtr<JPH::Character> character;
	bool bUseGroundState = false;
	float MaxSeparationDistance = 10.0f;
#endif // WITH_JOLT_PHYSICS
};
