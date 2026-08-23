#include "ThreatAssessment.h"

FThreatAssessment FCombatThreatAnalyzer::Evaluate(
	const FCombatPerceptionSnapshot& Snapshot,
	bool bDangerousWindowOccurred,
	float DistanceToTarget,
	float DangerRange) const
{
	FThreatAssessment Result;

	Result.bIsDangerous =
		bDangerousWindowOccurred &&
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