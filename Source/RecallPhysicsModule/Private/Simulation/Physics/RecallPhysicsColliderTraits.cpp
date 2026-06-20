// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Physics/RecallPhysicsColliderTraits.h"

#include "Landscape.h"
#include "Kismet/GameplayStatics.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityView.h"
#include "Simulation/Physics/RecallPhysicsColliderFragments.h"
#include "Utility/Physics/RecallPhysicsUtils.h"
#include "Utility/Trait/RecallTraitUtils.h"

//----------------------------------------------------------------------//
// URecallSphereCollisionTrait
//----------------------------------------------------------------------//
void URecallSphereCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FRecallPhysicsSphereShapeFragment SphereShapeFragment;
	SphereShapeFragment.Shape = FRecallPhysicsSphereShape(SphereRadius);
	SphereShapeFragment.Params = Params;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SphereShapeFragment));
}

//----------------------------------------------------------------------//
// URecallBoxCollisionTrait
//----------------------------------------------------------------------//
void URecallBoxCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FRecallPhysicsBoxShapeFragment BoxFragment;
	BoxFragment.Shape = FRecallPhysicsBoxShape(Extents);
	BoxFragment.Params = Params;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(BoxFragment));
}

//----------------------------------------------------------------------//
// URecallCapsuleCollisionTrait
//----------------------------------------------------------------------//
void URecallCapsuleCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FRecallPhysicsCapsuleFragment CapsuleFragment;
	CapsuleFragment.Shape = FRecallPhysicsCapsuleShape(Radius, HalfHeight);
	CapsuleFragment.Params = Params;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(CapsuleFragment));
}

//----------------------------------------------------------------------//
// URecallMeshCollisionTrait
//----------------------------------------------------------------------//
void URecallMeshCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.AddFragment<FRecallPhysicsMeshFragment>();

	FRecallPhysicsMeshConstSharedFragment MeshFragment;
	MeshFragment.Params = Params;
	MeshFragment.bConvexHull = bConvexHull;

	if (bUseActorScale)
	{
		BuildContext.GetMutableObjectFragmentInitializers().Add([this, &World](UObject& Owner, FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
			{
				if (const AActor* Actor = Recall::Trait::Utils::AsActor(Owner))
				{
					FRecallPhysicsMeshFragment& MeshFragment = EntityView.GetFragmentData<FRecallPhysicsMeshFragment>();
					MeshFragment.Scale = Actor->GetActorScale3D();
				}
			}
		);
	}

	Algo::Transform(Vertices, MeshFragment.Vertices, [](const FVector& Vertex)
	{
		return static_cast<FVector3f>(Vertex);
	});
	
	if (!bConvexHull)
	{
		MeshFragment.Triangles = Triangles;
		MeshFragment.MeshShapeSettings = MeshShapeSettings;
	}

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(MeshFragment));
}

#if WITH_EDITOR
void URecallMeshCollisionTrait::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
#if WITH_EDITORONLY_DATA
	SaveMesh();
#endif // WITH_EDITORONLY_DATA
}
#endif // WITH_EDITOR

void URecallMeshCollisionTrait::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	SaveMesh();
#endif // WITH_EDITORONLY_DATA
}

FVector URecallMeshCollisionTrait::GetExtents() const
{
	// TODO
	return Super::GetExtents();
}

void URecallMeshCollisionTrait::SaveMesh()
{
	TArray<FVector> NewVertices;
	TArray<int32> NewTriangles;
	Recall::Physics::Utils::SaveMesh(Mesh, NewVertices, NewTriangles);
	
	if (NewVertices != Vertices || NewTriangles != Triangles)
	{
		Vertices = NewVertices;
		Triangles = NewTriangles;
		
#if WITH_EDITORONLY_DATA
		VertexCount = NewVertices.Num();
		TriangleCount = NewTriangles.Num();
#endif // WITH_EDITORONLY_DATA
		
		Modify();
	}
}

//----------------------------------------------------------------------//
// URecallHeightFieldCollisionTrait
//----------------------------------------------------------------------//
void URecallHeightFieldCollisionTrait::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (!LandscapeActor)
	{
		LandscapeActor = Cast<ALandscape>(UGameplayStatics::GetActorOfClass(this, ALandscape::StaticClass()));
	}
#endif // WITH_EDITOR
}

void URecallHeightFieldCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FRecallPhysicsHeightFieldConstSharedFragment HeightFieldConstSharedFragment;
	HeightFieldConstSharedFragment.Params = Params;

	if (ensureAlwaysMsgf(LandscapeActor, TEXT("%hs No source landscape was set for this trait"), __FUNCTION__))
	{
		TArray<float> Heights;
		LandscapeActor->GetHeightValues(
			HeightFieldConstSharedFragment.Shape.SizeX, HeightFieldConstSharedFragment.Shape.SizeY, HeightFieldConstSharedFragment.Shape.Heights);
		HeightFieldConstSharedFragment.Shape.Scale = LandscapeActor->GetActorScale3D();
		
		HeightFieldConstSharedFragment.Location = LandscapeActor->GetActorLocation();
		HeightFieldConstSharedFragment.Rotation = LandscapeActor->GetActorRotation().Quaternion();
	}
		
	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(HeightFieldConstSharedFragment));
}
