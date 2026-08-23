#pragma once
#include "CoreMinimal.h"
#include "CombatMemory.generated.h"

USTRUCT(BlueprintType)
struct FCombatMemoryState
{
	GENERATED_BODY()

	// 0 = always dodges right, 1 = always dodges left, 0.5 = no clear bias yet
	UPROPERTY(BlueprintReadOnly)
	float LeftDodgeBias = 0.5f;

	// Rough 0-1 sense of how often the target is attacking
	UPROPERTY(BlueprintReadOnly)
	float AggressionEstimate = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 ObservedDodgeCount = 0;
};

class FIGHTINGGAME_API FCombatMemoryTracker
{
public:
	void Update(
		bool bTargetIsAttacking,
		bool bTargetJustDodged,
		const FVector& DodgeDirectionWorld,
		const FVector& ObserverForward,
		const FVector& ObserverRight,
		float DeltaTime);

	const FCombatMemoryState& GetState() const { return State; }

private:
	FCombatMemoryState State;

	float AggressionDecayRate = 0.15f;
	float DodgeBiasDecayRate = 0.25f;

	bool bWasDodgingLastTick = false;
};