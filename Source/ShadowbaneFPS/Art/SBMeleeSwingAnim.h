// Copyright shadowbanefps.
// Procedural axe / blade swing poses (no skeletal montage required for BasicShapes pawns).

#pragma once

#include "CoreMinimal.h"

/** Evaluates a readable axe-chop / blade-swing pose over normalized time 0..1. */
namespace SBMeleeSwingAnim
{
	inline float Smooth01(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	/** Full swing length in seconds (windup + strike + recover). */
	inline constexpr float DurationSeconds = 0.42f;

	/**
	 * FPS arm / weapon pivot rotation for an overhead-to-side axe chop.
	 * Pitch = raise/lower, Yaw = across body, Roll = blade twist.
	 */
	inline FRotator EvalFpPivot(float Alpha01)
	{
		const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
		const FRotator Idle(8.f, 18.f, 12.f);
		if (A <= 0.f)
		{
			return Idle;
		}
		if (A < 0.30f)
		{
			const float U = Smooth01(A / 0.30f);
			return FMath::Lerp(Idle, FRotator(-35.f, -55.f, 18.f), U);
		}
		if (A < 0.55f)
		{
			const float U = Smooth01((A - 0.30f) / 0.25f);
			const float Power = U * U;
			return FMath::Lerp(FRotator(-35.f, -55.f, 18.f), FRotator(28.f, 70.f, -8.f), Power);
		}
		const float U = Smooth01((A - 0.55f) / 0.45f);
		return FMath::Lerp(FRotator(28.f, 70.f, -8.f), Idle, U);
	}

	/** Third-person hand-mounted chop — moderate arc (body stays mostly still). */
	inline FRotator EvalTpPivot(float Alpha01)
	{
		const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
		const FRotator Idle(0.f, 0.f, 0.f);
		if (A <= 0.f)
		{
			return Idle;
		}
		if (A < 0.30f)
		{
			const float U = Smooth01(A / 0.30f);
			return FMath::Lerp(Idle, FRotator(-25.f, -40.f, 20.f), U);
		}
		if (A < 0.55f)
		{
			const float U = Smooth01((A - 0.30f) / 0.25f);
			const float Power = U * U;
			return FMath::Lerp(FRotator(-25.f, -40.f, 20.f), FRotator(30.f, 55.f, -5.f), Power);
		}
		const float U = Smooth01((A - 0.55f) / 0.45f);
		return FMath::Lerp(FRotator(30.f, 55.f, -5.f), Idle, U);
	}

	inline bool IsStrikeWindow(float Alpha01)
	{
		return Alpha01 >= 0.30f && Alpha01 <= 0.55f;
	}
}
