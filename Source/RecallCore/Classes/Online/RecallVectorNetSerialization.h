// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"

#include "RecallVectorNetSerialization.generated.h"

template<int32 MaxValue, uint32 NumBits>
bool SerializeFixedVector2D(FVector2d& Vector, FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		bool success = true;
		success &= WriteFixedCompressedFloat<MaxValue, NumBits>(Vector.X, Ar);
		success &= WriteFixedCompressedFloat<MaxValue, NumBits>(Vector.Y, Ar);
		return success;
	}
	else
	{
		ReadFixedCompressedFloat<MaxValue, NumBits>(Vector.X, Ar);
		ReadFixedCompressedFloat<MaxValue, NumBits>(Vector.Y, Ar);
		return true;
	}
}

template<int32 ScaleFactor, int32 MaxBitsPerComponent>
bool SerializePackedVector2D(FVector2D& Value, FArchive& Ar)
{
	Ar.UsingCustomVersion(FEngineNetworkCustomVersion::Guid);

	if (Ar.EngineNetVer() >= FEngineNetworkCustomVersion::PackedVectorLWCSupport && Ar.EngineNetVer() != FEngineNetworkCustomVersion::Ver21AndViewPitchOnly_DONOTUSE)
	{
		FVector3d DValue(Value.X, Value.Y, 0.0);
		const bool bRet = UE::Net::SerializeQuantizedVector<ScaleFactor>(DValue, Ar);
		Value = FVector2D(DValue.X, DValue.Y);
		return bRet;
	}
	else
	{
		check(Ar.IsLoading());
		FVector3f AsFloat(Value.X, Value.Y, 0.0f);
		const bool bRet = UE::Net::Private::LegacyReadPackedVector<ScaleFactor, MaxBitsPerComponent>(AsFloat, Ar);
		Value = FVector2D(AsFloat.X, AsFloat.Y);
		return bRet;
	}
}

/**
 *	FVector2D_NetQuantizeDirection
 *
 *	8 bits per component
 *	Valid range: -1..+1 inclusive
 */
USTRUCT()
struct FVector2D_NetQuantizeDirection : public FVector2D
{
	GENERATED_USTRUCT_BODY()

	FORCEINLINE FVector2D_NetQuantizeDirection()
	{}

	explicit FORCEINLINE FVector2D_NetQuantizeDirection(EForceInit E)
	: FVector2D(E)
	{}

	FORCEINLINE FVector2D_NetQuantizeDirection(double InX, double InY)
	: FVector2D(InX, InY)
	{}

	FORCEINLINE FVector2D_NetQuantizeDirection(const FVector2D &InVec)
	{
		FVector2D::operator=(InVec);
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = SerializeFixedVector2D<1, 8>(*this, Ar);
		return true;
	}

	bool operator==(const FVector2D_NetQuantizeDirection& Other) const
	{
		return FVector2D::operator==(Other);
	}

	bool operator!=(const FVector2D_NetQuantizeDirection& Other) const
	{
		return !operator==(Other);
	}
};

template<>
struct TStructOpsTypeTraits< FVector2D_NetQuantizeDirection > : public TStructOpsTypeTraitsBase2< FVector2D_NetQuantizeDirection >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

/**
 *	FVector2D_NetQuantize
 *
 *	20 bits per component
 *	Valid range: 2^20 = +/- 1,048,576
 */
USTRUCT()
struct FVector2D_NetQuantize : public FVector2D
{
	GENERATED_USTRUCT_BODY()

	FORCEINLINE FVector2D_NetQuantize()
	{}

	explicit FORCEINLINE FVector2D_NetQuantize(EForceInit E)
	: FVector2D(E)
	{}

	FORCEINLINE FVector2D_NetQuantize(double InX, double InY)
	: FVector2D(InX, InY)
	{}

	FORCEINLINE FVector2D_NetQuantize(const FVector2D &InVec)
	{
		FVector2D::operator=(InVec);
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = SerializePackedVector2D<1, 20>(*this, Ar);
		return true;
	}

	bool operator==(const FVector2D_NetQuantize& Other) const
	{
		return FVector2D::operator==(Other);
	}

	bool operator!=(const FVector2D_NetQuantize& Other) const
	{
		return !operator==(Other);
	}

	FVector ToDVector() const { return FVector(X, Y, 0.0); }
};

template<>
struct TStructOpsTypeTraits< FVector2D_NetQuantize > : public TStructOpsTypeTraitsBase2< FVector2D_NetQuantize >
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};