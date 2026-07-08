// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Physics/RecallPhysicsUtils.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Physics/JPRPhysicsLayerDataAsset.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "Landscape.h"
#include "MassCommandBuffer.h"
#include "MassEntityManager.h"
#include "MassEntityUtils.h"
#include "MassEntityView.h"
#include "MassExecutionContext.h"
#include "ProceduralMeshComponent.h"
#include "Physics/RecallPhysicsObjects.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsSensorFragment.h"
#include "Simulation/Sensor/RecallSensorFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Physics/RecallPhysicsSubsystem.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
static bool bDebugShowColliders = false;
static FAutoConsoleVariableRef CVarRecallDebugShowColliders(
	TEXT("recall.physics.ShowColliders"),
	bDebugShowColliders,
	TEXT("Show Colliders")
);

static bool bDebugShowInfos = false;
static FAutoConsoleVariableRef CVarRecallDebugShowVehiclePhysicsInfo(
	TEXT("recall.physics.ShowDebugInfos"),
	bDebugShowInfos,
	TEXT("Show Debug Infos")
);

static bool bDebugDumpPhysicsObject = false;
static FAutoConsoleVariableRef CVarRecallDumpPhysicsObject(
	TEXT("recall.physics.DumpPhysicsObject"),
	bDebugDumpPhysicsObject,
	TEXT("Dump Physics Object")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

namespace Recall::Physics::Utils
{
	
void GenerateMeshAlongSpline(const USplineComponent* SplineComponent, const FBox& Bounds,
	TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, float Interval,
	ESplineCoordinateSpace::Type CoordinateSpace, ERecallSplineMeshFace Faces, FFloatInterval DistanceRange, bool bFlipFaces)
{
	if (!IsValid(SplineComponent))
	{
		return;
	}

	const float SplineStartDistance = DistanceRange.Min * SplineComponent->GetSplineLength();
	const float SplineEndDistance = DistanceRange.Max * SplineComponent->GetSplineLength();
	const float SplineSectionLength = SplineEndDistance - SplineStartDistance;
	const float NumSegment = FMath::CeilToInt(SplineSectionLength / Interval);
	if (NumSegment == 0)
	{
		return;
	}
	
	constexpr int32 PrevTopLeftIndex		= -8;
	constexpr int32 PrevBottomLeftIndex		= -7;
	constexpr int32 PrevBottomRightIndex	= -6;
	constexpr int32 PrevTopRightIndex		= -5;
	
	constexpr int32 CurrTopLeftIndex		= -4;
	constexpr int32 CurrBottomLeftIndex		= -3;
	constexpr int32 CurrBottomRightIndex	= -2;
	constexpr int32 CurrTopRightIndex		= -1;

	auto PushTriangle = [&OutVertices, &OutTriangles, bFlipFaces](int32 V1, int32 V2, int32 V3)
	{
		if (bFlipFaces)
		{
			OutTriangles.Add(OutVertices.Num() + V1);
			OutTriangles.Add(OutVertices.Num() + V2);
			OutTriangles.Add(OutVertices.Num() + V3);
		}
		else
		{
			OutTriangles.Add(OutVertices.Num() + V3);
			OutTriangles.Add(OutVertices.Num() + V2);
			OutTriangles.Add(OutVertices.Num() + V1);
		}
	};
	
	const float SegmentLength = SplineSectionLength / NumSegment;

	FVector BoundsCenter, BoundsExtents;
	Bounds.GetCenterAndExtents(BoundsCenter, BoundsExtents);

	const FVector TopLeftOffset			= BoundsCenter + FVector(0.0f, -BoundsExtents.Y, BoundsExtents.Z);
	const FVector BottomLeftOffset		= BoundsCenter + FVector(0.0f, -BoundsExtents.Y, -BoundsExtents.Z);
	const FVector BottomRightOffset		= BoundsCenter + FVector(0.0f, BoundsExtents.Y, -BoundsExtents.Z);
	const FVector TopRightOffset		= BoundsCenter + FVector(0.0f, BoundsExtents.Y, BoundsExtents.Z);

	OutVertices.Reserve(OutVertices.Num() + NumSegment * 4);

	// Count triangles
	int32 TriangleCount = 0;

	for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
	{
		if (EnumHasAnyFlags(Faces, static_cast<ERecallSplineMeshFace>(1 << FaceIndex)))
		{
			TriangleCount += (NumSegment - 1) * 2;
		}
	}
	
	if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Front))
	{
		TriangleCount += 2;
	}	
	if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Back))
	{
		TriangleCount += 2;
	}
	
	OutTriangles.Reserve(OutTriangles.Num() + TriangleCount * 3);
	
	for (int32 SegmentIndex = 0; SegmentIndex <= NumSegment; ++SegmentIndex)
	{
		const float SegmentDistance = SplineStartDistance + SegmentIndex * SegmentLength;	
		FTransform SegmentTransform = SplineComponent->GetTransformAtDistanceAlongSpline(SegmentDistance, CoordinateSpace, true);

		// Ignore up vector from transform
		const FVector ForwardVector = SegmentTransform.GetRotation().GetForwardVector();
		const FVector RightVector = FVector::CrossProduct(FVector::UpVector, ForwardVector);
		SegmentTransform.SetRotation(UKismetMathLibrary::MakeRotationFromAxes(ForwardVector, RightVector, FVector::UpVector).Quaternion());

		OutVertices.Add(SegmentTransform.TransformPosition(TopLeftOffset));		// TL (-4)
		OutVertices.Add(SegmentTransform.TransformPosition(BottomLeftOffset));		// BL (-3)
		OutVertices.Add(SegmentTransform.TransformPosition(BottomRightOffset));	// BR (-2)
		OutVertices.Add(SegmentTransform.TransformPosition(TopRightOffset));		// TR (-1)

		if (SegmentIndex == 0)
		{
			// Front face
			if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Front))
			{
				PushTriangle(CurrTopLeftIndex, CurrTopRightIndex, CurrBottomLeftIndex);
				PushTriangle(CurrTopRightIndex, CurrBottomRightIndex, CurrBottomLeftIndex);
			}
			continue;
		}

		// Top face
		if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Top))
		{
			PushTriangle(CurrTopLeftIndex, CurrTopRightIndex, PrevTopLeftIndex);
			PushTriangle(CurrTopRightIndex, PrevTopRightIndex, PrevTopLeftIndex);
		}
		
		// Bottom face
		if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Bottom))
		{
			PushTriangle(PrevBottomLeftIndex, CurrBottomRightIndex, CurrBottomLeftIndex);
			PushTriangle(PrevBottomLeftIndex, PrevBottomRightIndex, CurrBottomRightIndex);
		}
		
		// Left face
		if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Left))
		{
			PushTriangle(CurrTopLeftIndex, PrevTopLeftIndex, CurrBottomLeftIndex);
			PushTriangle(PrevTopLeftIndex, PrevBottomLeftIndex, CurrBottomLeftIndex);
		}
		
		// Right face
		if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Right))
		{
			PushTriangle(PrevTopRightIndex, CurrTopRightIndex, PrevBottomRightIndex);
			PushTriangle(CurrTopRightIndex, CurrBottomRightIndex, PrevBottomRightIndex);
		}
		
		if (SegmentIndex == NumSegment)
		{
			// Back face
			if (EnumHasAnyFlags(Faces, ERecallSplineMeshFace::Back))
			{
				PushTriangle(CurrBottomLeftIndex, CurrTopRightIndex, CurrTopLeftIndex);
				PushTriangle(CurrBottomLeftIndex, CurrBottomRightIndex, CurrTopRightIndex);
			}
		}
	}
}
	
void SaveMesh(const TObjectPtr<UStaticMesh>& Mesh, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles)
{
	FStaticMeshRenderData* RenderData = IsValid(Mesh) ? Mesh->GetRenderData() : nullptr;
	if (RenderData != nullptr && RenderData->LODResources.Num() > 0)
	{
		const FStaticMeshLODResources& LODResources = RenderData->LODResources.Last();

		const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();

		OutTriangles.SetNum(IndexBuffer.Num());

		constexpr int32 BatchSize = 1000;

		ParallelFor(IndexBuffer.Num() / BatchSize + 1, [&OutTriangles, &IndexBuffer](int32 BatchIndex)
		{
			const int32 StartIndex = BatchIndex * BatchSize;
			const int32 EndIndex = FMath::Min(StartIndex + BatchSize, IndexBuffer.Num()) - 1;
			
			for (int32 Index = StartIndex; Index <= EndIndex; ++Index)
			{
				OutTriangles[Index] = IndexBuffer[Index];
			}
		});

		const FPositionVertexBuffer& PositionVertexBuffers = LODResources.VertexBuffers.PositionVertexBuffer;

		OutVertices.SetNum(PositionVertexBuffers.GetNumVertices());

		ParallelFor(PositionVertexBuffers.GetNumVertices() / BatchSize + 1, [&OutVertices, &PositionVertexBuffers](int32 BatchIndex)
		{
			const int32 StartIndex = BatchIndex * BatchSize;
			const int32 EndIndex = FMath::Min(StartIndex + BatchSize, static_cast<int32>(PositionVertexBuffers.GetNumVertices())) - 1;
			for (int32 Index = StartIndex; Index <= EndIndex; ++Index)
			{
				OutVertices[Index] = static_cast<FVector>(PositionVertexBuffers.VertexPosition(Index));
			}
		});
	}
}

void CreateLandscapeShape(URecallPhysicsSubsystem& PhysicsSystem, const ALandscape& Landscape, float Friction)
{
	FJPRPhysicsHeightFieldShape LandscapeShape;
	LandscapeShape.Scale = Landscape.GetActorScale3D();
	Landscape.GetHeightValues(LandscapeShape.SizeX, LandscapeShape.SizeY, LandscapeShape.Heights);
	
	if (!LandscapeShape.IsValid())
	{
		UE_LOG(LogRecallPhysics, Warning, TEXT("%hs Invalid landscape: %s"), __FUNCTION__, *Landscape.GetName());
		return;
	}

	PhysicsSystem.CreateStaticShape(LandscapeShape,
		Landscape.GetActorLocation(), Landscape.GetActorRotation().Quaternion(), Friction);
}
	
void CreateStaticMeshShapes(URecallPhysicsSubsystem& PhysicsSystem, float Friction)
{
	for (TActorIterator<AStaticMeshActor> ActorIt(PhysicsSystem.GetWorld()); ActorIt; ++ActorIt)
	{
		CreateStaticMeshEntity(PhysicsSystem, *ActorIt, Friction);
	}
}
	
void CreateStaticMeshEntity(URecallPhysicsSubsystem& PhysicsSystem, AStaticMeshActor* StaticMeshActor, float Friction)
{
	if (!IsValid(StaticMeshActor) || !StaticMeshActor->GetActorEnableCollision() || !StaticMeshActor->IsRootComponentStatic())
	{
		return;	
	}
	
	const UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
	if (!IsValid(StaticMeshComponent) || !StaticMeshComponent->IsPhysicsCollisionEnabled() ||
		StaticMeshComponent->GetCollisionObjectType() != ECollisionChannel::ECC_WorldStatic)
	{
		return;
	}
	
	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	if (!IsValid(StaticMesh))
	{
		return;
	}

	const FTransform MeshScaleTransform(FRotator::ZeroRotator, FVector::ZeroVector,
		StaticMeshActor->GetActorScale3D());

	const int32 MinLODIndex = StaticMesh->GetMinLOD().Default;
	const int32 NumSections = StaticMesh->GetNumSections(MinLODIndex);
	
	TArray<FVector> SectionVertices;
	TArray<int32> SectionTriangles;
	TArray<FVector> SectionNormals;
	TArray<FVector2D> SectionUVs;
	TArray<FProcMeshTangent> SectionTangents;

	FJPRPhysicsMeshShape MeshShape;
		
	for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
	{
		const int32 TriangleOffset = MeshShape.Vertices.Num();
			
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(StaticMesh, MinLODIndex, SectionIndex,
			SectionVertices, SectionTriangles, SectionNormals, SectionUVs, SectionTangents);
		
		// Scale mesh
		MeshShape.Vertices.Reserve(MeshShape.Vertices.Num() + SectionVertices.Num());
		Algo::Transform(SectionVertices, MeshShape.Vertices,
			[&MeshScaleTransform](const FVector& Vertex)
		{
			return static_cast<FVector3f>(MeshScaleTransform.TransformPosition(static_cast<FVector>(Vertex)));
		});

		MeshShape.Triangles.Reserve(MeshShape.Triangles.Num() + SectionTriangles.Num());
		Algo::Transform(SectionTriangles, MeshShape.Triangles,
			[TriangleOffset](const int32 Triangle)
		{
			return TriangleOffset + Triangle;
		});
	}

	if (MeshShape.Vertices.Num() == 0 || MeshShape.Triangles.Num() == 0)
	{
		return;
	}
	
	PhysicsSystem.CreateStaticShape(MeshShape,
		StaticMeshActor->GetActorLocation(), StaticMeshActor->GetActorRotation().Quaternion(), Friction);
}
	
void GenerateSplineMeshAlongSpline(
	AActor* OwnerActor,
	const TObjectPtr<UStaticMesh>& StaticMesh,
	const TObjectPtr<USplineComponent>& SplineComponent,
	TArray<TObjectPtr<USplineMeshComponent>>& OutSplineMeshComponents)
{
	if (!IsValid(OwnerActor) || !StaticMesh || !SplineComponent)
	{
		return;
	}
	
	const FBox MeshBoundingBox = StaticMesh->GetBoundingBox();
	const FVector MeshExtents = MeshBoundingBox.GetExtent();
	
	const float SplineLength = SplineComponent->GetSplineLength();
	const float NumMeshSegments = FMath::CeilToInt(SplineLength / MeshExtents.X);
	const float SplineMeshLength = SplineLength / NumMeshSegments;
	
	const int32 NumSplinePoints = SplineComponent->GetNumberOfSplinePoints();

	OutSplineMeshComponents.Empty(NumSplinePoints);
	
	for (int32 Segment = 0; Segment < NumMeshSegments; Segment++)
	{
		const FString SegmentName = FString::Printf(TEXT("SplineMeshComponent_%d"), Segment);
		
		const float StartDistance = Segment * SplineMeshLength;
		const float EndDistance = (Segment + 1) * SplineMeshLength;
		
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(OwnerActor, *SegmentName, RF_Transactional);
		SplineMesh->SetMobility(EComponentMobility::Static);
		SplineMesh->SetupAttachment(SplineComponent);
		SplineMesh->RegisterComponent();
		
		SplineMesh->SetStaticMesh(StaticMesh);
		SplineMesh->bCastDynamicShadow = false;

		// Width of the mesh
		const FVector PointScaleStart = SplineComponent->GetScaleAtDistanceAlongSpline(StartDistance);
		const FVector PointScaleEnd = SplineComponent->GetScaleAtDistanceAlongSpline(EndDistance);
		
		SplineMesh->SetStartScale(FVector2D(PointScaleStart.Y, PointScaleStart.Z), false);
		SplineMesh->SetEndScale(FVector2D(PointScaleEnd.Y, PointScaleEnd.Z), false);
		
		const FVector PointLocationStart = SplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const FVector PointLocationEnd = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		
		const FVector PointTangentStart = SplineComponent->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const FVector PointTangentEnd = SplineComponent->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		
		SplineMesh->SetStartAndEnd(PointLocationStart, PointTangentStart, PointLocationEnd, PointTangentEnd, false);
		SplineMesh->UpdateMesh();

		OutSplineMeshComponents.Add(SplineMesh);	
	}
}

void InitializePhysicsBody(FMassExecutionContext& Context, const FMassEntityHandle& Entity,
	URecallPhysicsSubsystem& PhysicsSystem, const FRecallPhysicsBodyFragment& BodyFragment, const FJPRPhysicsBodyParameters& BodyParams)
{
	const FRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetMutableBody(BodyFragment.BodyHandle);

	if (ensureMsgf(PhysicsBody.IsValid(), TEXT("Body does not exist.")) == false)
	{
		return;
	}

	if (BodyFragment.ShouldStartEnabled())
	{
		PhysicsBody.Pin()->Activate();
		PhysicsBody.SetLinearVelocity(BodyFragment.StartVelocity);
	}
	else
	{
		PhysicsBody.Pin()->Desactivate();
	}

	const FMassEntityView EntityView(Context.GetEntityManagerChecked(), Entity);
	if (FRecallPhysicsSensorFragment* SensorContainerFragmentPtr = EntityView.GetFragmentDataPtr<FRecallPhysicsSensorFragment>())
	{
		const FRecallSensorConstSharedFragment* SensorSharedFragmentPtr = EntityView.GetConstSharedFragmentDataPtr<FRecallSensorConstSharedFragment>();
		if (SensorSharedFragmentPtr != nullptr)
		{
			FRecallSensorFragment& SensorFragment = EntityView.GetFragmentData<FRecallSensorFragment>();
			
			check(SensorFragment.BodyHandles.IsEmpty());
			SensorFragment.BodyHandles.SetNum(SensorSharedFragmentPtr->InstanceParameters.Num());

			for (int32 SensorIndex = 0; SensorIndex < SensorSharedFragmentPtr->InstanceParameters.Num(); SensorIndex++)
			{
				const FRecallSensorInstanceParameters& SensorParameters = SensorSharedFragmentPtr->InstanceParameters[SensorIndex];

				SensorFragment.BodyHandles[SensorIndex] = PhysicsSystem.CreateShape(Entity, SensorParameters.Shape, SensorParameters.Params);
				SensorContainerFragmentPtr->AddSensor(SensorFragment.BodyHandles[SensorIndex], SensorParameters.SensorName);
			}
		}
		
		if (BodyParams.bIsSensor)
		{
			SensorContainerFragmentPtr->AddSensor(BodyFragment.BodyHandle);
		}
	
		if (SensorContainerFragmentPtr->IsSensor())
		{
			Context.Defer().AddTag<FRecallPhysicsSensorTag>(Entity);
		}
	}
			
	if (BodyFragment.ShouldGenerateHitEvent())
	{
		Context.Defer().AddTag<FRecallPhysicsGeneratesHitEventTag>(Entity);
	}
}

void Teleport(const FMassEntityManager& EntityManager, const FMassEntityHandle& Entity,
	URecallPhysicsSubsystem& PhysicsSubsystem, const FVector& NewPosition)
{
	const FMassEntityView EntityView(EntityManager, Entity);
	
	// Update transform fragment
	if (FRecallTransformFragment* TransformFragment = EntityView.GetFragmentDataPtr<FRecallTransformFragment>())
	{
		TransformFragment->Position = NewPosition;
	}
	
	// Update physics body if present
	if (const FRecallPhysicsBodyFragment* PhysicsBodyFragment = EntityView.GetFragmentDataPtr<FRecallPhysicsBodyFragment>())
	{
		const FRecallPhysicsBodyView PhysicsBody = PhysicsSubsystem.GetMutableBody(PhysicsBodyFragment->BodyHandle);
		if (PhysicsBody.IsValid())
		{
			PhysicsBody.SetPosition(NewPosition);
		}
	}
}
	
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT	
bool ShouldDebugShowColliders()
{
	return bDebugShowColliders;
}
	
bool ShouldDebugShowInfos()
{
	return bDebugShowInfos;
}
	
bool ShouldDebugDumpPhysicsObject()
{
	return bDebugDumpPhysicsObject;
}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	
} // namespace Recall::Physics::Utils
