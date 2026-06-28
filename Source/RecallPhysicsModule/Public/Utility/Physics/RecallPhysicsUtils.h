// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"

class ALandscape;
class AStaticMeshActor;
class USplineComponent;
class USplineMeshComponent;
struct FMassEntityHandle;
struct FMassEntityManager;
struct FMassExecutionContext;
struct FRecallPhysicsBodyFragment;
struct FJPRPhysicsBodyParameters;

namespace ESplineCoordinateSpace
{
	enum Type : int;
}

class URecallPhysicsSubsystem;

UENUM(BlueprintType, meta=(BitFlags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ERecallSplineMeshFace : uint8
{
	None			= 0			UMETA(Hidden),

	Top				= 1 << 0,
	Bottom			= 1 << 1,
	Left			= 1 << 2,
	Right			= 1 << 3,
	Front			= 1 << 4,
	Back			= 1 << 5,

	All				= Top | Bottom | Left | Right | Front | Back
};
ENUM_CLASS_FLAGS(ERecallSplineMeshFace)

#define RECALL_SPLINE_MESH_FACE_COUNT 6
#define RECALL_SPLINE_MESH_ALL_FACE_EXCEPT_BOTTOM\
	ERecallSplineMeshFace::Top |\
	ERecallSplineMeshFace::Left |\
	ERecallSplineMeshFace::Right |\
	ERecallSplineMeshFace::Front |\
	ERecallSplineMeshFace::Back

namespace Recall::Physics::Utils
{
	
/// Extract the data of a mesh, only works in the Editor.
/// @param Mesh The mesh
/// @param OutVertices The vertices of this mesh that have been extracted
/// @param OutTriangles The triangles of this mesh that have been extracted
extern RECALLPHYSICSMODULE_API void SaveMesh(const TObjectPtr<UStaticMesh>& Mesh, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles);
	
/// Generate a mesh along a spline.
/// @param SplineComponent The spline to use to generate the mesh.
/// @param Bounds The bounds of the mesh to generate along the spline, the X-axis of the extents won't be used.
/// @param OutVertices The vertices of the generated mesh.
/// @param OutTriangles The triangle indices of the generate mesh.
/// @param Interval How many centimeters between each vertex on the X-axis.
/// @param CoordinateSpace The spline coordinate space used to generate the mesh.
/// @param Faces The faces of the mesh to generate.
/// @param DistanceRange The range of the spline to use to generate the mesh (0.0f is the start and 1.0f the end of the spline).
/// @param bFlipFaces Flip all the faces of the meshes.
extern RECALLPHYSICSMODULE_API void GenerateMeshAlongSpline(
	const USplineComponent* SplineComponent, const FBox& Bounds, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles,
	float Interval = 100.0f, ESplineCoordinateSpace::Type CoordinateSpace = ESplineCoordinateSpace::Type::World,
	ERecallSplineMeshFace Faces = ERecallSplineMeshFace::All, FFloatInterval DistanceRange = { 0.0f, 1.0f }, bool bFlipFaces = false);

/// Spawn an entity for the landscape.
/// @param PhysicsSystem The physics system which will contains this new physics body.
/// @param Landscape The landscape to generate the entity from.
/// @param Friction The friction of the physics body for the new landscape entity.
extern RECALLPHYSICSMODULE_API void CreateLandscapeShape(URecallPhysicsSubsystem& PhysicsSystem, const ALandscape& Landscape, float Friction = 0.2f);

/// Spawn entities for all the static mesh actors in the world.
/// @param PhysicsSystem The physics system which will contains this new physics bodies.
/// @param Friction The friction of the physics body for the new entities.
extern RECALLPHYSICSMODULE_API void CreateStaticMeshShapes(URecallPhysicsSubsystem& PhysicsSystem, float Friction = 0.2f);
extern RECALLPHYSICSMODULE_API void CreateStaticMeshEntity(URecallPhysicsSubsystem& PhysicsSystem, AStaticMeshActor* StaticMeshActor, float Friction = 0.2f);

/**
 * Helper method to setup a newly created physics body's base values from its fragments.
 */
extern RECALLPHYSICSMODULE_API void InitializePhysicsBody(FMassExecutionContext& Context, const FMassEntityHandle& Entity,
	URecallPhysicsSubsystem& PhysicsSystem, const FRecallPhysicsBodyFragment& BodyFragment, const FJPRPhysicsBodyParameters& BodyParams);

/**
 * Teleports an entity to a new position, updating both transform fragment and physics body
 */
extern RECALLPHYSICSMODULE_API void Teleport(const FMassEntityManager& EntityManager, const FMassEntityHandle& Entity,
	URecallPhysicsSubsystem& PhysicsSubsystem, const FVector& NewPosition);
	
/// Generate spline mesh components along a spline mesh.
/// @param OwnerActor The actor which will own the new components.
/// @param StaticMesh The state mesh to use for the spline mesh components.
/// @param SplineComponent The spline to generate the spline mesh component along.
/// @param OutSplineMeshComponents The generated spline mesh components.
extern RECALLPHYSICSMODULE_API void GenerateSplineMeshAlongSpline(
	AActor* OwnerActor,
	const TObjectPtr<UStaticMesh>& StaticMesh,
	const TObjectPtr<USplineComponent>& SplineComponent,
	TArray<TObjectPtr<USplineMeshComponent>>& OutSplineMeshComponents);
	
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
extern RECALLPHYSICSMODULE_API bool ShouldDebugShowColliders();
extern RECALLPHYSICSMODULE_API bool ShouldDebugShowInfos();
extern RECALLPHYSICSMODULE_API bool ShouldDebugDumpPhysicsObject();
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	
} // namespace Recall::Physics::Utils
