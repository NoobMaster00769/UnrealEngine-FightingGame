#include "EnemyBrainComponent.h"
#include "EnemyRoleDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UEnemyBrainComponent::UEnemyBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyBrainComponent::BeginPlay()
{
	Super::BeginPlay();
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
	if (Context.TargetActor == nullptr)
	{
		return;
	}

	Context.DistanceToTarget =
		FVector::Distance(
			GetOwner()->GetActorLocation(),
			Context.TargetActor->GetActorLocation()
		);
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

	return
		(Context.DistanceToTarget - P.PreferredCombatDistance)
		* P.ApproachWeight;
}

float UEnemyBrainComponent::ScoreRetreat() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile& P = GetProfile();

	if (Context.DistanceToTarget >= P.RetreatDistance)
		return 0.f;

	return
		(P.RetreatDistance - Context.DistanceToTarget)
		* P.RetreatWeight;
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

	return
		100.f *
		P.StrafeWeight;
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
	OnDecisionMade.Broadcast(CurrentDecision.Action);
}