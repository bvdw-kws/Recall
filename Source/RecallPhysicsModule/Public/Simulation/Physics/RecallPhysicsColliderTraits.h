// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "RecallPhysicsBodyTrait.h"
#include "Physics/Common/RecallPhysicsCommonShapeTypes.h"

#include "RecallPhysicsColliderTraits.generated.h"

UCLASS(meta=(DisplayName="MS Sphere Collider"))
class RECALLPHYSICSMODULE_API URecallSphereCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape, meta=(Units="Centimeters"))
	float SphereRadius = 50.0f;

protected:
	virtual FVector GetExtents() const override final { return static_cast<FVector>(SphereRadius); }

};

UCLASS(meta=(DisplayName="MS Box Collider"))
class RECALLPHYSICSMODULE_API URecallBoxCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()

public:
	void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape, meta=(Units="Centimeters"))
	FVector Extents = (FVector)100.0f;

protected:
	virtual FVector GetExtents() const override final { return Extents; }

};

UCLASS(meta=(DisplayName="MS Capsule Collider"))
class RECALLPHYSICSMODULE_API URecallCapsuleCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape, meta=(Units="Centimeters"))
	float Radius = 34.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape, meta=(Units="Centimeters"), DisplayName="HalfHeightOfCylinder")
	float HalfHeight = 88.0f;

protected:
	virtual FVector GetExtents() const override final { return FVector(Radius, Radius, HalfHeight + Radius); }

};

UCLASS(meta=(DisplayName="MS Mesh Collider"))
class RECALLPHYSICSMODULE_API URecallMeshCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

public:
	virtual void PostLoad() override;

protected:
	virtual FVector GetExtents() const override final;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape)
	TObjectPtr<UStaticMesh> Mesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape)
	bool bConvexHull = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape, meta=(EditCondition="!bConvexHull", ShowOnlyInnerProperties))
	FRecallPhysicsMeshShapeSettings MeshShapeSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Shape)
	bool bUseActorScale = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category=Mesh)
	int32 VertexCount = 0;
	
	UPROPERTY(VisibleAnywhere, Category=Mesh)
	int32 TriangleCount = 0;
#endif // WITH_EDITORONLY_DATA
	
	UPROPERTY()
	TArray<FVector> Vertices;
	
	UPROPERTY()
	TArray<int32> Triangles;

	UFUNCTION(CallInEditor, Category=Editor)
	void SaveMesh();
};

/**
 * Landscape collider entity.
 */
UCLASS(meta=(DisplayName="MS Height Field"))
class RECALLPHYSICSMODULE_API URecallHeightFieldCollisionTrait : public URecallPhysicsBodyTrait
{
	GENERATED_BODY()
	
public:
	virtual void PostLoad() override;

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditInstanceOnly)
	TObjectPtr<class ALandscape> LandscapeActor;
};
