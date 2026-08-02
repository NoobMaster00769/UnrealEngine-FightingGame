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

// Plain C++ helper, not a UActorComponent - it has no lifecycle of its own,
// it just converts a Perception snapshot + distance into a judgment.
class FIGHTINGGAME_API FCombatThreatAnalyzer
{
public:
	FThreatAssessment Evaluate(
		const FCombatPerceptionSnapshot& Snapshot,
		float DistanceToTarget,
		float DangerRange) const;
};