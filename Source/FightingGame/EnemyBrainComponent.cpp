#include "EnemyBrainComponent.h"
#include "EnemyRoleDataAsset.h"
#include "Engine/World.h"
#include "CombatComponent.h"
#include "DefenseComponent.h"
#include "HealthComponent.h"
#include "HitReactionComponent.h"
#include "EnemyMovementComponent.h"
#include "CombatPerceptionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "CombatDirectorSubsystem.h"
#include "GameFramework/Actor.h"

UEnemyBrainComponent::UEnemyBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	Defense = GetOwner()->FindComponentByClass<UDefenseComponent>();
	Health = GetOwner()->FindComponentByClass<UHealthComponent>();
	Movement = GetOwner()->FindComponentByClass<UEnemyMovementComponent>();
	HitReaction = GetOwner()->FindComponentByClass<UHitReactionComponent>();
	Perception = GetOwner()->FindComponentByClass<UCombatPerceptionComponent>();

	if (Combat)
	{
		Combat->OnAttackEnded.AddDynamic(this, &UEnemyBrainComponent::HandleOwnAttackEnded);
	}
}

void UEnemyBrainComponent::HandleOwnAttackEnded()
{
	if (UWorld* World = GetWorld())
	{
		if (UCombatDirectorSubsystem* Director = World->GetSubsystem<UCombatDirectorSubsystem>())
		{
			Director->ReleaseAttackToken(GetOwner());
		}
	}
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

	if (Perception && !Perception->GetSnapshot().bTargetNoticed)
	{
		if (Movement)
		{
			Movement->StopMovement();
		}
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
			if (UWorld* World = GetWorld())
			{
				if (UCombatDirectorSubsystem* Director = World->GetSubsystem<UCombatDirectorSubsystem>())
				{
					Director->ReportDodgeBiasObservation(CurrentMemory.LeftDodgeBias);
				}
			}
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

			UE_LOG(LogTemp, Warning,
				TEXT("[Scores] App=%.0f Ret=%.0f Str=%.0f Lgt=%.0f Hvy=%.0f Dog=%.0f Cnt=%.0f Wait=%.0f Token=%d"),
				ScoreApproach(), ScoreRetreat(), ScoreStrafe(), ScoreLightAttack(),
				ScoreHeavyAttack(), ScoreDodge(), ScoreCounter(), ScoreWait(), bHasAttackToken);
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

	if (UWorld* World = GetWorld())
	{
		if (UCombatDirectorSubsystem* Director = World->GetSubsystem<UCombatDirectorSubsystem>())
		{
			Director->RegisterActiveEnemy(GetOwner());
		}
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
	const ECombatAction PreviousAction = CurrentDecision.Action;

	bHasAttackToken = false;

	if (UWorld* World = GetWorld())
	{
		if (UCombatDirectorSubsystem* Director = World->GetSubsystem<UCombatDirectorSubsystem>())
		{
			bHasAttackToken = Director->TryAcquireAttackToken(GetOwner());
		}
	}

	auto Stick = [PreviousAction](ECombatAction Action, float Score) -> float
		{
			return (Action == PreviousAction) ? Score * 1.15f : Score;
		};

	CurrentDecision.Action = ECombatAction::Wait;
	CurrentDecision.Score = Stick(ECombatAction::Wait, ScoreWait());

	float Score = Stick(ECombatAction::Approach, ScoreApproach());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::Approach; }

	Score = Stick(ECombatAction::Retreat, ScoreRetreat());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::Retreat; }

	Score = Stick(ECombatAction::Strafe, ScoreStrafe());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::Strafe; }

	Score = Stick(ECombatAction::LightAttack, ScoreLightAttack());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::LightAttack; }

	Score = Stick(ECombatAction::HeavyAttack, ScoreHeavyAttack());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::HeavyAttack; }

	Score = Stick(ECombatAction::Dodge, ScoreDodge());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::Dodge; }

	Score = Stick(ECombatAction::Counter, ScoreCounter());
	if (Score > CurrentDecision.Score) { CurrentDecision.Score = Score; CurrentDecision.Action = ECombatAction::Counter; }

	const bool bCommittedToAttack =
		CurrentDecision.Action == ECombatAction::LightAttack ||
		CurrentDecision.Action == ECombatAction::HeavyAttack ||
		CurrentDecision.Action == ECombatAction::Counter;

	if (!bCommittedToAttack)
	{
		if (UWorld* World = GetWorld())
		{
			if (UCombatDirectorSubsystem* Director = World->GetSubsystem<UCombatDirectorSubsystem>())
			{
				Director->ReleaseAttackToken(GetOwner());
			}
		}
	}
}

float UEnemyBrainComponent::ScoreApproach() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

	// If we have the token and can attack, keep closing until actually within
	// striking range - not just the general preferred distance. Otherwise we
	// stall in a dead zone: too far to attack, not far enough to trigger Approach.
	float EngageDistance = P.PreferredCombatDistance;

	if (bHasAttackToken && Context.bCanAttack)
	{
		EngageDistance = FMath::Min(P.PreferredCombatDistance, P.LightAttackRange * 0.85f);
	}

	if (Context.DistanceToTarget <= EngageDistance)
		return 0.f;

	float Score =
		(Context.DistanceToTarget - EngageDistance)
		* P.ApproachWeight;

	Score *= FMath::Lerp(0.5f, 1.3f, Context.EnemyHealthPercent);
	Score *= FMath::Lerp(1.f, 1.f - P.AggressionApproachPenaltyScale, CurrentMemory.AggressionEstimate);

	if (Context.bCanAttack) 
		Score *= GetNearbyAllyPenalty();

	return Score;
}

float UEnemyBrainComponent::ScoreRetreat() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

	if (Context.DistanceToTarget >= P.RetreatDistance)
		return 0.f;

	float Score =
		(P.RetreatDistance - Context.DistanceToTarget)
		* P.RetreatWeight;

	// longer by creating distance rather than committing to a bad trade.
	Score *= FMath::Lerp(2.5f, 1.f, Context.EnemyHealthPercent);

	return Score;
}

float UEnemyBrainComponent::ScoreStrafe() const
{
	if (!RoleAsset)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

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

	Score *= GetNearbyAllyPenalty();

	return Score;
}

float UEnemyBrainComponent::ScoreLightAttack() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	if (!bHasAttackToken)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

	if (Context.DistanceToTarget > P.LightAttackRange)
		return 0.f;

	if (Perception && Perception->GetSnapshot().bIsInvulnerable)
		return 0.f;

	float Score = 150.f * P.LightAttackWeight;

	if (CurrentThreat.bIsPunishOpportunity)
	{
		Score += P.PunishAttackBonus;
	}

	if (Perception && !Perception->GetSnapshot().bTargetIsFacingMe)
	{
		Score += P.BlindSideAttackBonus;
	}

	if (Context.TargetHealthPercent < 0.3f)
	{
		Score += P.ExecuteBonusScale * (1.f - Context.TargetHealthPercent);
	}

	return Score;
}

float UEnemyBrainComponent::ScoreHeavyAttack() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	if (!bHasAttackToken)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

	if (Context.DistanceToTarget > P.HeavyAttackRange)
		return 0.f;

	if (Perception && Perception->GetSnapshot().bIsInvulnerable)
		return 0.f;

	float Score = 120.f * P.HeavyAttackWeight;

	if (CurrentThreat.bIsPunishOpportunity)
	{
		Score += P.PunishAttackBonus;
	}

	if (Perception && !Perception->GetSnapshot().bTargetIsFacingMe)
	{
		Score += P.BlindSideAttackBonus;
	}

	if (Context.TargetHealthPercent < 0.3f)
	{
		Score += P.ExecuteBonusScale * (1.f - Context.TargetHealthPercent);
	}

	return Score;
}

float UEnemyBrainComponent::ScoreDodge() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanDodge)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

	float Score = 15.f * P.DodgeWeight;   

	if (CurrentThreat.bIsDangerous)
	{
		Score += P.DangerDodgeBonus;
	}

	// Standing caution against an aggressive target only makes sense once
	// they're actually close enough to matter.
	if (Context.DistanceToTarget <= ThreatDangerRange)
	{
		Score += CurrentMemory.AggressionEstimate * P.AggressionDodgeBonusScale;
	}

	return Score;
}

float UEnemyBrainComponent::ScoreWait() const
{
	if (!RoleAsset)
		return 1.f;

	const FRoleProfile P = GetEffectiveProfile();

	float Score = 10.f * P.WaitWeight;

	if (!bHasAttackToken && Context.DistanceToTarget > P.StrafeMinDistance)
	{
		Score *= 2.f;
	}

	return Score;
}

float UEnemyBrainComponent::ScoreCounter() const
{
	if (!RoleAsset)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	if (!Context.bCanAttack)
		return 0.f;

	if (!bHasAttackToken)
		return 0.f;

	if (!CurrentThreat.bIsPunishOpportunity)
		return 0.f;

	const FRoleProfile P = GetEffectiveProfile();

	if (Context.DistanceToTarget > P.LightAttackRange)
		return 0.f;

	return
		(150.f * P.LightAttackWeight) +
		P.CounterBonus;
}

bool UEnemyBrainComponent::ShouldContinueCombo() const
{
	if (!RoleAsset)
		return false;

	const FRoleProfile& P = GetProfile();

	if (CurrentThreat.bIsDangerous)
		return false;

	if (Context.DistanceToTarget > P.LightAttackRange)
		return false;

	if (!Context.bCanAttack)
		return false;

	return FMath::FRand() <= P.ComboLikelihood;
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
	{
		bool bUsedFlankSlot = false;

		if (UWorld* World = GetWorld())
		{
			if (UCombatDirectorSubsystem* Director = World->GetSubsystem<UCombatDirectorSubsystem>())
			{
				const float SlotAngleDeg = Director->GetFlankSlotAngle(GetOwner());
				const FRoleProfile P = GetEffectiveProfile();
				const float AngleRad = FMath::DegreesToRadians(SlotAngleDeg);

				const FVector Offset =
					FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.f) * P.PreferredCombatDistance;

				const FVector SlotPosition = Context.TargetActor->GetActorLocation() + Offset;

				Movement->MoveToFlankSlot(Context.TargetActor, SlotPosition);
				bUsedFlankSlot = true;
			}
		}

		if (!bUsedFlankSlot)
		{
			Movement->SetStrafePreference(CurrentMemory.LeftDodgeBias > 0.5f);
			Movement->StrafeAroundTarget(Context.TargetActor);
		}

		break;
	}

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

void UEnemyBrainComponent::ApplyRuntimeOverride(const FRoleProfileOverride& Override)
{
	RuntimeOverride = Override;
	RuntimeOverride.DangerDodgeBonusMult = FMath::Clamp(RuntimeOverride.DangerDodgeBonusMult, 0.5f, 2.f);
	RuntimeOverride.PunishAttackBonusMult = FMath::Clamp(RuntimeOverride.PunishAttackBonusMult, 0.5f, 2.f);
	RuntimeOverride.CounterBonusMult = FMath::Clamp(RuntimeOverride.CounterBonusMult, 0.5f, 2.f);
}

FRoleProfile UEnemyBrainComponent::GetEffectiveProfile() const
{
	FRoleProfile Effective = GetProfile();

	if (RuntimeOverride.bActive)
	{
		Effective.DangerDodgeBonus *= RuntimeOverride.DangerDodgeBonusMult;
		Effective.PunishAttackBonus *= RuntimeOverride.PunishAttackBonusMult;
		Effective.CounterBonus *= RuntimeOverride.CounterBonusMult;
	}

	return Effective;
}

float UEnemyBrainComponent::GetNearbyAllyPenalty() const
{
	if (!GetWorld() || !GetOwner())
		return 1.f;

	TArray<AActor*> AllEnemies;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

	int32 NearbyAllies = 0;

	for (AActor* Other : AllEnemies)
	{
		if (Other && Other != GetOwner())
		{
			if (FVector::Dist(Other->GetActorLocation(), GetOwner()->GetActorLocation()) < AllyProximityRadius)
			{
				NearbyAllies++;
			}
		}
	}

	return FMath::Max(1.f - (NearbyAllies * 0.25f), 0.25f);
}