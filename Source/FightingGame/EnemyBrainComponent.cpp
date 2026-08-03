#include "EnemyBrainComponent.h"
#include "EnemyRoleDataAsset.h"
#include "Engine/World.h"
#include "CombatComponent.h"
#include "DefenseComponent.h"
#include "HealthComponent.h"
#include "HitReactionComponent.h"
#include "EnemyMovementComponent.h"
#include "CombatPerceptionComponent.h"
#include "GameFramework/Actor.h"

UEnemyBrainComponent::UEnemyBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	Combat =
		GetOwner()->FindComponentByClass<UCombatComponent>();

	Defense =
		GetOwner()->FindComponentByClass<UDefenseComponent>();

	Health =
		GetOwner()->FindComponentByClass<UHealthComponent>();

	Movement =
		GetOwner()->FindComponentByClass<UEnemyMovementComponent>();

	HitReaction =
		GetOwner()->FindComponentByClass<UHitReactionComponent>();

	Perception =
		GetOwner()->FindComponentByClass<UCombatPerceptionComponent>();
}

void UEnemyBrainComponent::InitializeBrain()
{
	if (RoleAsset == nullptr)
		return;

	EnemyRole = RoleAsset->Role;
}

void UEnemyBrainComponent::StartThinking()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		ThinkTimer,
		this,
		&UEnemyBrainComponent::Think,
		ThinkInterval,
		true
	);
}

void UEnemyBrainComponent::StopThinking()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(ThinkTimer);
}

void UEnemyBrainComponent::Think()
{
	if (!Health || !Health->IsAlive())
	{
		return;
	}

	if (Combat && Combat->IsAttacking())
	{
		return;
	}

	if (HitReaction && HitReaction->IsReacting())
	{
		return;
	}

	if (Defense && Defense->IsDodging())
	{
		return;
	}

	UpdateContext();

	if (Perception)
	{
		const FCombatPerceptionSnapshot& S = Perception->GetSnapshot();

		UE_LOG(LogTemp, Warning,
			TEXT("[Brain->Perception] Recovering=%d CollisionActive=%d TargetFacingMe=%d"),
			S.bIsRecovering, S.bWeaponCollisionActive, S.bTargetIsFacingMe);

		const bool bDangerLatch = Perception->ConsumeDangerLatch();

		CurrentThreat = ThreatAnalyzer.Evaluate(
			S,
			bDangerLatch,
			Context.DistanceToTarget,
			ThreatDangerRange);

		if (UDefenseComponent* TargetDefense =
			Context.TargetActor
			? Context.TargetActor->FindComponentByClass<UDefenseComponent>()
			: nullptr)
		{
			MemoryTracker.Update(
				S.bIsAttacking,
				S.bIsDodging,
				TargetDefense->GetDodgeDirection(),
				GetOwner()->GetActorForwardVector(),
				GetOwner()->GetActorRightVector(),
				ThinkInterval);

			CurrentMemory = MemoryTracker.GetState();
		}

		if (bDebugBrain)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Threat] Dangerous=%d PunishOpportunity=%d Level=%.1f"),
				CurrentThreat.bIsDangerous,
				CurrentThreat.bIsPunishOpportunity,
				CurrentThreat.ThreatLevel);

			UE_LOG(LogTemp, Warning,
				TEXT("[Memory] LeftDodgeBias=%.2f Aggression=%.2f DodgesSeen=%d"),
				CurrentMemory.LeftDodgeBias,
				CurrentMemory.AggressionEstimate,
				CurrentMemory.ObservedDodgeCount);
		}

	}
	EvaluateActions();

	ExecuteDecision();
}

void UEnemyBrainComponent::SetTarget(AActor* NewTarget)
{
	Context.TargetActor = NewTarget;

	if (Perception)
	{
		Perception->SetTarget(NewTarget);
	}

	if (Movement)
	{
		Movement->SetTarget(NewTarget);
	}
}

void UEnemyBrainComponent::UpdateContext()
{
	if (!Context.TargetActor)
	{
		return;
	}

	Context.DistanceToTarget =
		FVector::Distance(
			GetOwner()->GetActorLocation(),
			Context.TargetActor->GetActorLocation());

	if (Health)
	{
		Context.EnemyHealthPercent =
			Health->GetHealthPercent();
	}

	if (Combat)
	{
		Context.bCanAttack =
			Combat->CanAttack();
	}

	if (Defense)
	{
		Context.bCanDodge =
			Defense->CanDodge();
	}

	if (UHealthComponent* TargetHealth =
		Context.TargetActor->FindComponentByClass<UHealthComponent>())
	{
		Context.TargetHealthPercent =
			TargetHealth->GetHealthPercent();
	}

	if (bDebugBrain)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Brain] Dist %.0f  HP %.2f"),
			Context.DistanceToTarget,
			Context.EnemyHealthPercent);
	}
}

const FRoleProfile& UEnemyBrainComponent::GetProfile() const
{
	return RoleAsset->RoleProfile;
}

void UEnemyBrainComponent::EvaluateActions()
{
	CurrentDecision.Action = ECombatAction::Wait;
	CurrentDecision.Score = ScoreWait();

	float Score = 0.f;

	Score = ScoreApproach();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::Approach;
	}

	Score = ScoreRetreat();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::Retreat;
	}

	Score = ScoreStrafe();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::Strafe;
	}

	Score = ScoreLightAttack();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::LightAttack;
	}

	Score = ScoreHeavyAttack();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::HeavyAttack;
	}

	Score = ScoreDodge();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::Dodge;
	}

	Score = ScoreCounter();
	if (Score > CurrentDecision.Score)
	{
		CurrentDecision.Score = Score;
		CurrentDecision.Action = ECombatAction::Counter;
	}
}

float UEnemyBrainComponent::ScoreApproach() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget <= P.PreferredCombatDistance)
		return 0.f;

	float Score =
		(Context.DistanceToTarget - P.PreferredCombatDistance)
		* P.ApproachWeight;

	Score *=
		FMath::Lerp(
			0.5f,
			1.3f,
			Context.EnemyHealthPercent);

	// Less eager to close distance against a consistently aggressive target.
	Score *= FMath::Lerp(1.f, 1.f - P.AggressionApproachPenaltyScale, CurrentMemory.AggressionEstimate);

	return Score;
}

float UEnemyBrainComponent::ScoreRetreat() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget >= P.RetreatDistance)
		return 0.f;

	float Score =
		(P.RetreatDistance - Context.DistanceToTarget)
		* P.RetreatWeight;

	Score *=
		FMath::Lerp(
			2.0f,
			0.4f,
			Context.EnemyHealthPercent);

	return Score;
}

float UEnemyBrainComponent::ScoreStrafe() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget < P.StrafeMinDistance)
		return 0.f;

	if (Context.DistanceToTarget > P.StrafeMaxDistance)
		return 0.f;

	float Score =
		100.f * P.StrafeWeight;

	Score *=
		FMath::Lerp(
			0.7f,
			1.2f,
			Context.EnemyHealthPercent);

	return Score;
}

float UEnemyBrainComponent::ScoreLightAttack() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget > P.LightAttackRange)
		return 0.f;

	float Score = 150.f * P.LightAttackWeight;

	if (CurrentThreat.bIsPunishOpportunity)
	{
		Score += P.PunishAttackBonus;
	}

	return Score;
}

float UEnemyBrainComponent::ScoreHeavyAttack() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget > P.HeavyAttackRange)
		return 0.f;

	float Score =
		120.f *
		P.HeavyAttackWeight;

	if (CurrentThreat.bIsPunishOpportunity)
	{
		Score += P.PunishAttackBonus;
	}

	return Score;
}

float UEnemyBrainComponent::ScoreDodge() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanDodge)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	float Score =
		30.f *
		P.DodgeWeight;

	if (CurrentThreat.bIsDangerous)
	{
		Score += P.DangerDodgeBonus;
	}

	Score += CurrentMemory.AggressionEstimate * P.AggressionDodgeBonusScale;

	// A target that's been consistently aggressive earns a standing
	// caution bonus, not just a reaction to an active swing.

	return Score;
}

float UEnemyBrainComponent::ScoreWait() const
{
	if (!RoleAsset)
		return 1.f;

	return
		10.f *
		GetProfile().WaitWeight;
}

float UEnemyBrainComponent::ScoreCounter() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	if (!CurrentThreat.bIsPunishOpportunity)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget > P.LightAttackRange)
		return 0.f;

	return
		(150.f * P.LightAttackWeight) +
		P.CounterBonus;
}

void UEnemyBrainComponent::ExecuteDecision()
{
	if (bDebugBrain)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Brain] Decision = %s"),
			*UEnum::GetValueAsString(CurrentDecision.Action));
	}

	if (!Movement)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Brain] EnemyMovementComponent Missing"));
		return;
	}

	switch (CurrentDecision.Action)
	{
	case ECombatAction::Approach:

		Movement->ApproachTarget(Context.TargetActor);
		break;

	case ECombatAction::Retreat:

		Movement->RetreatFromTarget(Context.TargetActor);
		break;

	case ECombatAction::Strafe:

		// crowd their favored side: strafe toward the side they dodge into.
		Movement->SetStrafePreference(CurrentMemory.LeftDodgeBias > 0.5f);

		Movement->StrafeAroundTarget(Context.TargetActor);
		break;

	case ECombatAction::Wait:

		Movement->StopMovement();
		break;

	case ECombatAction::LightAttack:

		if (Combat && Combat->CanAttack())
		{
			Movement->StopMovement();
			Combat->StartLightAttack();
		}

		break;

	case ECombatAction::HeavyAttack:

		if (Combat && Combat->CanAttack())
		{
			Movement->StopMovement();
			Combat->StartHeavyAttack();
		}

		break;

	case ECombatAction::Dodge:
	{
		Movement->StopMovement();

		if (Defense)
		{
			const FVector ToPlayer =
				(Context.TargetActor->GetActorLocation() -
					GetOwner()->GetActorLocation()).GetSafeNormal();

			const FVector Right =
				FVector::CrossProduct(
					FVector::UpVector,
					ToPlayer);

			bLastDodgeWasLeft = !bLastDodgeWasLeft;

			const FVector DodgeDirection =
				bLastDodgeWasLeft ? -Right : Right;

			Defense->StartDodge(DodgeDirection);
		}

		break;
	}

	case ECombatAction::Counter:

		if (Combat && Combat->CanAttack())
		{
			Movement->StopMovement();
			Combat->StartLightAttack();
		}

		break;

	default:

		Movement->StopMovement();
		break;
	}

	OnDecisionMade.Broadcast(CurrentDecision.Action);
}