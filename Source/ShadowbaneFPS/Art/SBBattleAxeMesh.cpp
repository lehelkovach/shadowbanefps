// Copyright shadowbanefps.

#include "Art/SBBattleAxeMesh.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"

namespace
{
	int32 AddVert(TArray<FVector>& Verts, const FVector& V)
	{
		return Verts.Add(V);
	}

	void AddTri(TArray<int32>& Tris, int32 A, int32 B, int32 C)
	{
		Tris.Add(A);
		Tris.Add(B);
		Tris.Add(C);
	}

	void AddQuad(TArray<int32>& Tris, int32 A, int32 B, int32 C, int32 D)
	{
		AddTri(Tris, A, B, C);
		AddTri(Tris, A, C, D);
	}
}

void SBBattleAxeMesh::Build(UProceduralMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> Colors;

	const float Scale = 100.f; // local meters → cm
	auto V = [Scale](float X, float Y, float Z) { return FVector(X, Y, Z) * Scale; };

	const float HaftLen = 1.05f;
	const float HaftR = 0.028f;
	constexpr int32 Sides = 8;

	// Haft cylinder along +Z
	TArray<int32> RingLo, RingHi;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float Ang = (static_cast<float>(i) / Sides) * TWO_PI;
		const float Cx = FMath::Cos(Ang) * HaftR;
		const float Cy = FMath::Sin(Ang) * HaftR;
		RingLo.Add(AddVert(Verts, V(Cx, Cy, 0.f)));
		RingHi.Add(AddVert(Verts, V(Cx, Cy, HaftLen)));
	}
	for (int32 i = 0; i < Sides; ++i)
	{
		const int32 I2 = (i + 1) % Sides;
		AddQuad(Tris, RingLo[i], RingLo[I2], RingHi[I2], RingHi[i]);
	}

	// Pommel
	const float PommelZ = -0.04f;
	const float PommelR = 0.045f;
	const int32 PommelBot = AddVert(Verts, V(0.f, 0.f, PommelZ - 0.03f));
	TArray<int32> PommelTop;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float Ang = (static_cast<float>(i) / Sides) * TWO_PI;
		PommelTop.Add(AddVert(Verts, V(FMath::Cos(Ang) * PommelR, FMath::Sin(Ang) * PommelR, PommelZ)));
	}
	for (int32 i = 0; i < Sides; ++i)
	{
		const int32 I2 = (i + 1) % Sides;
		AddTri(Tris, PommelBot, PommelTop[I2], PommelTop[i]);
		AddQuad(Tris, RingLo[i], RingLo[I2], PommelTop[I2], PommelTop[i]);
	}

	// Socket collar
	const float SocketZ = HaftLen - 0.02f;
	const float SocketR = 0.04f;
	const float SocketH = 0.12f;
	TArray<int32> SockLo, SockHi;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float Ang = (static_cast<float>(i) / Sides) * TWO_PI;
		SockLo.Add(AddVert(Verts, V(FMath::Cos(Ang) * SocketR, FMath::Sin(Ang) * SocketR, SocketZ - SocketH * 0.5f)));
		SockHi.Add(AddVert(Verts, V(FMath::Cos(Ang) * SocketR * 1.05f, FMath::Sin(Ang) * SocketR * 1.05f, SocketZ + SocketH * 0.5f)));
	}
	for (int32 i = 0; i < Sides; ++i)
	{
		const int32 I2 = (i + 1) % Sides;
		AddQuad(Tris, SockLo[i], SockLo[I2], SockHi[I2], SockHi[i]);
	}

	// Crescent / bearded battle-axe blade (+X) — wide curved edge, not a pick spike
	constexpr int32 NBlade = 14;
	TArray<int32> InnerMid, OuterMid;
	for (int32 i = 0; i < NBlade; ++i)
	{
		const float T = static_cast<float>(i) / static_cast<float>(NBlade - 1);
		const float Z = SocketZ + 0.22f - T * 0.55f;
		const float Flare = 0.18f + 0.22f * FMath::Sin(T * PI * 0.85f) + 0.08f * T;
		const float Xi = 0.05f;
		const float Xo = Xi + Flare + 0.06f * FMath::Sin(T * PI);
		InnerMid.Add(AddVert(Verts, V(Xi, 0.f, Z)));
		OuterMid.Add(AddVert(Verts, V(Xo, 0.f, Z)));
	}

	auto Extrude = [&](const TArray<int32>& Chain, float YOff)
	{
		TArray<int32> Out;
		Out.Reserve(Chain.Num());
		for (const int32 Idx : Chain)
		{
			const FVector P = Verts[Idx];
			Out.Add(AddVert(Verts, FVector(P.X, YOff * Scale, P.Z)));
		}
		return Out;
	};

	const TArray<int32> InnerP = Extrude(InnerMid, 0.02f);
	const TArray<int32> InnerN = Extrude(InnerMid, -0.02f);
	const TArray<int32> OuterP = Extrude(OuterMid, 0.01f);
	const TArray<int32> OuterN = Extrude(OuterMid, -0.01f);

	for (int32 i = 0; i < NBlade - 1; ++i)
	{
		AddQuad(Tris, InnerP[i], OuterP[i], OuterP[i + 1], InnerP[i + 1]);
		AddQuad(Tris, InnerN[i + 1], OuterN[i + 1], OuterN[i], InnerN[i]);
		AddQuad(Tris, OuterP[i], OuterN[i], OuterN[i + 1], OuterP[i + 1]);
		AddQuad(Tris, InnerN[i], InnerP[i], InnerP[i + 1], InnerN[i + 1]);
	}
	AddQuad(Tris, InnerP[0], InnerN[0], OuterN[0], OuterP[0]);
	AddQuad(Tris, InnerN[NBlade - 1], InnerP[NBlade - 1], OuterP[NBlade - 1], OuterN[NBlade - 1]);

	// Short rear poll / hammer face (battle axe), deliberately NOT a long pickaxe spike
	const float PollLen = 0.09f;
	const float PollZ0 = SocketZ - 0.04f;
	const float PollZ1 = SocketZ + 0.04f;
	const float PollY = 0.025f;
	const int32 P0 = AddVert(Verts, V(-0.02f, -PollY, PollZ0));
	const int32 P1 = AddVert(Verts, V(-0.02f - PollLen, -PollY, PollZ0));
	const int32 P2 = AddVert(Verts, V(-0.02f - PollLen, PollY, PollZ0));
	const int32 P3 = AddVert(Verts, V(-0.02f, PollY, PollZ0));
	const int32 P4 = AddVert(Verts, V(-0.02f, -PollY, PollZ1));
	const int32 P5 = AddVert(Verts, V(-0.02f - PollLen, -PollY, PollZ1));
	const int32 P6 = AddVert(Verts, V(-0.02f - PollLen, PollY, PollZ1));
	const int32 P7 = AddVert(Verts, V(-0.02f, PollY, PollZ1));
	AddQuad(Tris, P0, P1, P2, P3);
	AddQuad(Tris, P4, P7, P6, P5);
	AddQuad(Tris, P0, P3, P7, P4);
	AddQuad(Tris, P1, P5, P6, P2);
	AddQuad(Tris, P3, P2, P6, P7);
	AddQuad(Tris, P0, P4, P5, P1);

	Normals.SetNum(Verts.Num());
	UV0.SetNum(Verts.Num());
	Colors.SetNum(Verts.Num());
	Tangents.SetNum(Verts.Num());
	for (int32 i = 0; i < Verts.Num(); ++i)
	{
		Normals[i] = FVector::UpVector;
		UV0[i] = FVector2D(Verts[i].X * 0.01f, Verts[i].Z * 0.01f);
		Colors[i] = FLinearColor(0.7f, 0.72f, 0.76f, 1.f);
		Tangents[i] = FProcMeshTangent(FVector::RightVector, false);
	}

	Mesh->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UV0, Colors, Tangents, false);
	Mesh->ContainsPhysicsTriMeshData(false);
	if (UMaterialInterface* Mat = GetFallbackMaterial())
	{
		Mesh->SetMaterial(0, Mat);
	}
}

UMaterialInterface* SBBattleAxeMesh::GetFallbackMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
	if (MID)
	{
		const FLinearColor Steel(0.62f, 0.64f, 0.68f);
		MID->SetVectorParameterValue(TEXT("Color"), Steel);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Steel);
		MID->SetScalarParameterValue(TEXT("Metallic"), 0.55f);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
		Cached = MID;
	}
	return Cached.Get();
}
