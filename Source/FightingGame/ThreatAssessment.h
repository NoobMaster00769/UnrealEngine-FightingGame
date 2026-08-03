#pragma once
#include "CoreMinimal.h"
#include "CombatPerceptionComponent.h"
#include "ThreatAssessment.generated.h"

USTRUCT(BlueprintType)
struct FThreatAssessment
{
	GENERATED_BODY()

	// Player is mid-swing with an active hitbox, within range -> do not approach/attack blindly
	UPROPERTY(BlueprintReadOnly)
	bool bIsDangerous = false;

	// Player is in post-swing recovery -> opening to punish
	UPROPERTY(BlueprintReadOnly)
	bool bIsPunishOpportunity = false;

	// Single scalar summarizing the above, for later utility scoring.
	// Positive = threat outweighs opportunity, negative = opportunity outweighs threat.
	UPROPERTY(BlueprintReadOnly)
	float ThreatLevel = 0.f;
};

class FIGHTINGGAME_API FCombatThreatAnalyzer
{
public:
	FThreatAssessment Evaluate(
		const FCombatPerceptionSnapshot& Snapshot,
		bool bDangerousWindowOccurred,
		float DistanceToTarget,
		float DangerRange) const;
};