#include "CombatMemory.h"

void FCombatMemoryTracker::Update(
	bool bTargetIsAttacking,
	bool bTargetJustDodged,
	const FVector& DodgeDirectionWorld,
	const FVector& ObserverForward,
	const FVector& ObserverRight,
	float DeltaTime)
{
	// Aggression: EWMA toward 1 if attacking this tick, toward 0 if not.
	const float AggressionTarget = bTargetIsAttacking ? 1.f : 0.f;
	const float AggressionAlpha =
		FMath::Clamp(AggressionDecayRate * DeltaTime, 0.f, 1.f);

	State.AggressionEstimate =
		FMath::Lerp(State.AggressionEstimate, AggressionTarget, AggressionAlpha);

	// Dodge bias: only sample once per dodge event, not every tick of the dodge.
	if (bTargetJustDodged && !bWasDodgingLastTick)
	{
		const float RightAmount =
			FVector::DotProduct(DodgeDirectionWorld.GetSafeNormal(), ObserverRight);

		// RightAmount > 0 means dodge was to the observer's right,
		// < 0 means to the observer's left.
		const float LeftSample = RightAmount < 0.f ? 1.f : 0.f;

		State.LeftDodgeBias =
			FMath::Lerp(State.LeftDodgeBias, LeftSample, DodgeBiasDecayRate);

		State.ObservedDodgeCount++;
	}

	bWasDodgingLastTick = bTargetJustDodged;
}