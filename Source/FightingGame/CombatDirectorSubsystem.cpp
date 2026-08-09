#include "CombatDirectorSubsystem.h"
#include "CombatComponent.h"
#include "GameFramework/Actor.h"

void UCombatDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	InWorld.GetTimerManager().SetTimer(
		SampleTimer,
		this,
		&UCombatDirectorSubsystem::SampleDistances,
		SampleInterval,
		true);

	InWorld.GetTimerManager().SetTimer(
		BudgetTimer,
		this,
		&UCombatDirectorSubsystem::TickBudget,
		BudgetTickInterval,
		true);
}

void UCombatDirectorSubsystem::TickBudget()
{
	ElapsedTime += BudgetTickInterval;

	const float TensionMultiplier =
		1.f + TensionAmplitude * FMath::Sin(2.f * PI * ElapsedTime / TensionPeriodSeconds);

	CurrentBudget = FMath::Min(
		CurrentBudget + (BudgetGrowthPerSecond * TensionMultiplier * BudgetTickInterval),
		MaxBudget);
}

void UCombatDirectorSubsystem::ReportDodgeBiasObservation(float LeftBias)
{
	Stats.GlobalLeftDodgeBias = FMath::Lerp(Stats.GlobalLeftDodgeBias, LeftBias, 0.1f);
}

float UCombatDirectorSubsystem::GetRoleCost(EEnemyRole Role) const
{
	switch (Role)
	{
	case EEnemyRole::Coward:    return 1.f;
	case EEnemyRole::Duelist:   return 2.f;
	case EEnemyRole::Defender:  return 2.f;
	case EEnemyRole::Hunter:    return 3.f;
	case EEnemyRole::Aggressor: return 3.f;
	default:                    return 2.f;
	}
}

bool UCombatDirectorSubsystem::TrySpendBudget(EEnemyRole Role)
{
	const float Cost = GetRoleCost(Role);

	if (CurrentBudget < Cost)
	{
		return false;
	}

	CurrentBudget -= Cost;
	return true;
}

EEnemyRole UCombatDirectorSubsystem::ChooseNextRole() const
{
	TMap<EEnemyRole, float> Weights;
	Weights.Add(EEnemyRole::Aggressor, 1.f);
	Weights.Add(EEnemyRole::Defender, 1.f);
	Weights.Add(EEnemyRole::Duelist, 1.f);
	Weights.Add(EEnemyRole::Hunter, 1.f);
	Weights.Add(EEnemyRole::Coward, 1.f);

	// Player is pressuring hard (dealing damage, closing distance) -> counter with defensive roles.
	const bool bPlayerAggressive = Stats.DamageDealtEstimate > 15.f && Stats.PreferredDistance < 200.f;
	if (bPlayerAggressive)
	{
		Weights[EEnemyRole::Defender] += 1.5f;
		Weights[EEnemyRole::Coward] += 0.5f;
		Weights[EEnemyRole::Aggressor] -= 0.5f;
	}

	// Player is passive / keeping distance -> force engagement.
	const bool bPlayerPassive = Stats.PreferredDistance > 300.f;
	if (bPlayerPassive)
	{
		Weights[EEnemyRole::Aggressor] += 1.5f;
		Weights[EEnemyRole::Hunter] += 1.f;
	}

	// Player is precise -> harder-to-hit and punish-specialist roles.
	if (Stats.Accuracy > 0.7f)
	{
		Weights[EEnemyRole::Defender] += 0.75f;
		Weights[EEnemyRole::Hunter] += 0.75f;
	}

	float TotalWeight = 0.f;
	for (const auto& Pair : Weights)
	{
		TotalWeight += FMath::Max(Pair.Value, 0.f);
	}

	if (TotalWeight <= 0.f)
	{
		return EEnemyRole::Duelist;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);

	for (const auto& Pair : Weights)
	{
		const float W = FMath::Max(Pair.Value, 0.f);
		if (Roll <= W)
		{
			return Pair.Key;
		}
		Roll -= W;
	}

	return EEnemyRole::Duelist;
}

float UCombatDirectorSubsystem::GetFlankSlotAngle(AActor* Enemy) const
{
	TArray<AActor*> ValidEnemies;
	for (AActor* E : ActiveEnemies)
	{
		if (IsValid(E))
		{
			ValidEnemies.Add(E);
		}
	}

	// Stable sort by unique ID so each enemy keeps a consistent slot tick to
	// tick, and the roster only re-splits 360 degrees when someone joins/dies.
	ValidEnemies.Sort([](const AActor& A, const AActor& B)
		{
			return A.GetUniqueID() < B.GetUniqueID();
		});

	const int32 Index = ValidEnemies.IndexOfByKey(Enemy);

	if (Index == INDEX_NONE || ValidEnemies.Num() == 0)
	{
		return 0.f;
	}

	return (360.f / ValidEnemies.Num()) * Index;
}

void UCombatDirectorSubsystem::RegisterPlayerCombat(AActor* InPlayerActor, UCombatComponent* InPlayerCombat)
{
	if (!InPlayerActor || !InPlayerCombat)
	{
		return;
	}

	PlayerActor = InPlayerActor;
	PlayerCombat = InPlayerCombat;

	PlayerCombat->OnSuccessfulHit.AddDynamic(this, &UCombatDirectorSubsystem::HandlePlayerSuccessfulHit);
	PlayerCombat->OnAttackStarted.AddDynamic(this, &UCombatDirectorSubsystem::HandlePlayerAttackStarted);
}

void UCombatDirectorSubsystem::RegisterActiveEnemy(AActor* EnemyActor)
{
	if (!EnemyActor || ActiveEnemies.Contains(EnemyActor))
	{
		return;
	}

	ActiveEnemies.Add(EnemyActor);

	if (UCombatComponent* EnemyCombat = EnemyActor->FindComponentByClass<UCombatComponent>())
	{
		// Enemy successfully hitting the player -> damage taken by player.
		EnemyCombat->OnSuccessfulHit.AddDynamic(this, &UCombatDirectorSubsystem::HandleEnemySuccessfulHit);
	}
}

void UCombatDirectorSubsystem::UnregisterActiveEnemy(AActor* EnemyActor)
{
	ActiveEnemies.Remove(EnemyActor);
}
void UCombatDirectorSubsystem::HandlePlayerSuccessfulHit(AActor* HitActor)
{
	if (!PlayerCombat)
	{
		return;
	}

	const float Damage = PlayerCombat->GetCurrentDamage();

	// Guard against a hit registering after CurrentAttackDamage has already
	// reset (late overlap resolution) - a zero-damage "hit" isn't a real data point.
	if (Damage <= 0.f)
	{
		return;
	}

	Stats.DamageDealtEstimate = FMath::Lerp(Stats.DamageDealtEstimate, Damage, HitStatsDecayAlpha);

	AttacksLanded += 1.f;

	if (bDebugDirector)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Director] Player dealt %.1f dmg to %s"), Damage,
			HitActor ? *HitActor->GetName() : TEXT("Unknown"));
	}
}

void UCombatDirectorSubsystem::HandleEnemySuccessfulHit(AActor* HitActor)
{
	if (HitActor != PlayerActor)
	{
		return;
	}

	for (AActor* Enemy : ActiveEnemies)
	{
		if (UCombatComponent* EnemyCombat = Enemy ? Enemy->FindComponentByClass<UCombatComponent>() : nullptr)
		{
			if (EnemyCombat->IsAttacking())
			{
				const float Damage = EnemyCombat->GetCurrentDamage();

				Stats.DamageTakenEstimate = FMath::Lerp(Stats.DamageTakenEstimate, Damage, HitStatsDecayAlpha);

				if (bDebugDirector)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Director] Player took %.1f dmg from %s"), Damage, *Enemy->GetName());
				}
				break;
			}
		}
	}
}

void UCombatDirectorSubsystem::HandlePlayerAttackStarted()
{
	AttacksAttempted += 1.f;
}

void UCombatDirectorSubsystem::SampleDistances()
{
	if (!PlayerActor || ActiveEnemies.Num() == 0)
	{
		return;
	}

	float ClosestDistance = -1.f;

	for (AActor* Enemy : ActiveEnemies)
	{
		if (!Enemy)
		{
			continue;
		}

		const float Dist = FVector::Distance(PlayerActor->GetActorLocation(), Enemy->GetActorLocation());

		if (ClosestDistance < 0.f || Dist < ClosestDistance)
		{
			ClosestDistance = Dist;
		}
	}

	if (ClosestDistance < 0.f)
	{
		return;
	}

	const float Alpha = FMath::Clamp(StatsDecayRate * SampleInterval, 0.f, 1.f);
	Stats.PreferredDistance = FMath::Lerp(Stats.PreferredDistance, ClosestDistance, Alpha);

	if (AttacksAttempted > 0.f)
	{
		Stats.Accuracy = FMath::Clamp(AttacksLanded / AttacksAttempted, 0.f, 1.f);
	}

	if (bDebugDirector)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Director] DmgDealt=%.1f DmgTaken=%.1f Accuracy=%.2f PreferredDist=%.0f"),
			Stats.DamageDealtEstimate, Stats.DamageTakenEstimate, Stats.Accuracy, Stats.PreferredDistance);
	}
}

bool UCombatDirectorSubsystem::TryAcquireAttackToken(AActor* Enemy)
{
	ActiveAttackers.RemoveAll([](AActor* A) { return !IsValid(A); });

	if (ActiveAttackers.Contains(Enemy))
	{
		return true;
	}

	if (ActiveAttackers.Num() >= MaxConcurrentAttackers)
	{
		return false;
	}

	ActiveAttackers.Add(Enemy);
	return true;
}

void UCombatDirectorSubsystem::ReleaseAttackToken(AActor* Enemy)
{
	ActiveAttackers.Remove(Enemy);
}