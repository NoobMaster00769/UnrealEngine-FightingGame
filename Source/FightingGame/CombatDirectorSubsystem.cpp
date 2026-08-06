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