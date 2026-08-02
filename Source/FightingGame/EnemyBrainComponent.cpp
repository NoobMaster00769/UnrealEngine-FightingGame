#include "EnemyBrainComponent.h"
#include "EnemyRoleDataAsset.h"
#include "Engine/World.h"
#include "CombatComponent.h"
#include "DefenseComponent.h"
#include "HealthComponent.h"
#include "HitReactionComponent.h"
#include "EnemyMovementComponent.h"
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

	EvaluateActions();

	ExecuteDecision();
}

void UEnemyBrainComponent::SetTarget(AActor* NewTarget)
{
	Context.TargetActor = NewTarget;
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

	return
		150.f *
		P.LightAttackWeight;
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

	return
		120.f *
		P.HeavyAttackWeight;
}

float UEnemyBrainComponent::ScoreDodge() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanDodge)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	return
		30.f *
		P.DodgeWeight;
}

float UEnemyBrainComponent::ScoreWait() const
{
	if (!RoleAsset)
		return 1.f;

	return
		10.f *
		GetProfile().WaitWeight;
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

		Movement->StopMovement();

		// Defense->StartDodge() later

		break;

	default:

		Movement->StopMovement();
		break;
	}

	OnDecisionMade.Broadcast(CurrentDecision.Action);
}