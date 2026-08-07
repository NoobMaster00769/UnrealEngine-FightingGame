#pragma once

#include "CoreMinimal.h"
#include "RoleProfile.generated.h"

USTRUCT(BlueprintType)
struct FRoleProfile
{
	GENERATED_BODY()

	/* Distances */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances")
	float PreferredCombatDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances")
	float LightAttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances")
	float HeavyAttackRange = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances")
	float RetreatDistance = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances")
	float StrafeMinDistance = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances")
	float StrafeMaxDistance = 320.f;

	/* Action Weights */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float ApproachWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float RetreatWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float StrafeWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float WaitWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float LightAttackWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float HeavyAttackWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights")
	float DodgeWeight = 1.f;

	/* Personality */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	float Aggression = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	float ReactionSpeed = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	float ComboLikelihood = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
	float AttackCooldownMultiplier = 1.f;

	/* Reactive Tuning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactive")
	float DangerDodgeBonus = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactive")
	float PunishAttackBonus = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactive")
	float CounterBonus = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactive")
	float AggressionDodgeBonusScale = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactive")
	float AggressionApproachPenaltyScale = 0.5f;
};
USTRUCT(BlueprintType)
struct FRoleProfileOverride
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float DangerDodgeBonusMult = 1.f;

	UPROPERTY(BlueprintReadWrite)
	float PunishAttackBonusMult = 1.f;

	UPROPERTY(BlueprintReadWrite)
	float CounterBonusMult = 1.f;

	UPROPERTY(BlueprintReadWrite)
	bool bActive = false;
};
