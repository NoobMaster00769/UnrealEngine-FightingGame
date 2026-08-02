#include "ThreatAssessment.h"

FThreatAssessment FCombatThreatAnalyzer::Evaluate(
	const FCombatPerceptionSnapshot& Snapshot,
	float DistanceToTarget,
	float DangerRange) const
{
	FThreatAssessment Result;

	Result.bIsDangerous =
		Snapshot.bIsAttacking &&
		Snapshot.bWeaponCollisionActive &&
		DistanceToTarget <= DangerRange;

	Result.bIsPunishOpportunity =
		Snapshot.bIsRecovering;

	Result.ThreatLevel = 0.f;

	if (Result.bIsDangerous)
	{
		Result.ThreatLevel += 1.f;
	}

	if (Result.bIsPunishOpportunity)
	{
		Result.ThreatLevel -= 1.f;
	}

	return Result;
}