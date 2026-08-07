#include "WaveSpawnerActor.h"
#include "EnemyBrainComponent.h"
#include "CombatDirectorSubsystem.h"
#include "RoleProfile.h"
#include"HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

AWaveSpawnerActor::AWaveSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AWaveSpawnerActor::BeginPlay()
{
	Super::BeginPlay();
	StartNextWave();
}

void AWaveSpawnerActor::StartNextWave()
{
	if (bWaveInProgress)
	{
		return;
	}

	UCombatDirectorSubsystem* Director = GetWorld() ? GetWorld()->GetSubsystem<UCombatDirectorSubsystem>() : nullptr;
	if (!Director)
	{
		return;
	}

	TArray<UEnemyRoleDataAsset*> WaveComposition;

	for (int32 i = 0; i < MaxEnemiesPerWave; i++)
	{
		const EEnemyRole ChosenRole = Director->ChooseNextRole();

		if (!Director->TrySpendBudget(ChosenRole))
		{
			break;
		}

		if (UEnemyRoleDataAsset* Asset = FindRoleAsset(ChosenRole))
		{
			WaveComposition.Add(Asset);
		}
	}

	if (WaveComposition.Num() == 0)
	{
		// Budget can't afford even one enemy yet - check again shortly.
		GetWorld()->GetTimerManager().SetTimer(
			NextWaveTimer, this, &AWaveSpawnerActor::StartNextWave, TimeBetweenWaves, false);
		return;
	}

	CurrentWaveNumber++;
	bWaveInProgress = true;
	AliveCount = 0;

	if (bDebugSpawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Spawner] === Wave %d starting, %d enemies ==="),
			CurrentWaveNumber, WaveComposition.Num());
	}

	for (UEnemyRoleDataAsset* Asset : WaveComposition)
	{
		SpawnEnemy(Asset);
	}
}

void AWaveSpawnerActor::HandleEnemyDeath()
{
	AliveCount--;

	if (bDebugSpawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Spawner] Enemy died, %d remaining"), AliveCount);
	}

	if (AliveCount <= 0 && bWaveInProgress)
	{
		bWaveInProgress = false;

		if (bDebugSpawner)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave %d cleared, next in %.1fs"),
				CurrentWaveNumber, TimeBetweenWaves);
		}

		GetWorld()->GetTimerManager().SetTimer(
			NextWaveTimer, this, &AWaveSpawnerActor::StartNextWave, TimeBetweenWaves, false);
	}
}

AActor* AWaveSpawnerActor::GetRandomSpawnPoint() const
{
	if (SpawnPoints.Num() == 0)
	{
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, SpawnPoints.Num() - 1);
	return SpawnPoints[Index];
}

UEnemyRoleDataAsset* AWaveSpawnerActor::FindRoleAsset(EEnemyRole InRole) const
{
	for (UEnemyRoleDataAsset* Asset : RoleRotation)
	{
		if (Asset && Asset->Role == InRole)
		{
			return Asset;
		}
	}
	return nullptr;
}

void AWaveSpawnerActor::TryDirectorSpawn()
{
	UCombatDirectorSubsystem* Director = GetWorld() ? GetWorld()->GetSubsystem<UCombatDirectorSubsystem>() : nullptr;
	if (!Director)
	{
		return;
	}

	const EEnemyRole ChosenRole = Director->ChooseNextRole();

	if (!Director->TrySpendBudget(ChosenRole))
	{
		if (bDebugSpawner)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Spawner] Not enough budget for %s (have %.1f)"),
				*UEnum::GetValueAsString(ChosenRole), Director->GetCurrentBudget());
		}
		return;
	}

	if (UEnemyRoleDataAsset* Asset = FindRoleAsset(ChosenRole))
	{
		SpawnEnemy(Asset);
	}
}

void AWaveSpawnerActor::SpawnEnemy(UEnemyRoleDataAsset* RoleAsset)
{
	if (!EnemyClass || !RoleAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[Spawner] Missing EnemyClass or RoleAsset."));
		return;
	}

	AActor* SpawnPoint = GetRandomSpawnPoint();
	if (!SpawnPoint)
	{
		UE_LOG(LogTemp, Error, TEXT("[Spawner] No spawn points configured."));
		return;
	}

	const FTransform SpawnTransform = SpawnPoint->GetActorTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewEnemy = GetWorld()->SpawnActor<APawn>(EnemyClass, SpawnTransform, SpawnParams);
	if (!NewEnemy)
	{
		UE_LOG(LogTemp, Error, TEXT("[Spawner] SpawnActor failed."));
		return;
	}

	if (UEnemyBrainComponent* Brain = NewEnemy->FindComponentByClass<UEnemyBrainComponent>())
	{
		Brain->RoleAsset = RoleAsset;
		Brain->InitializeBrain();

		if (ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(this, 0))
		{
			Brain->SetTarget(PlayerChar);
		}

		Brain->StartThinking();

		if (UHealthComponent* EnemyHealth = NewEnemy->FindComponentByClass<UHealthComponent>())
		{
			EnemyHealth->OnDeath.AddDynamic(this, &AWaveSpawnerActor::HandleEnemyDeath);
		}

		AliveCount++;

		if (UCombatDirectorSubsystem* Director = GetWorld()->GetSubsystem<UCombatDirectorSubsystem>())
		{
			if (FMath::FRand() < EliteChance)
			{
				FRoleProfileOverride Override;
				Override.bActive = true;
				Override.DangerDodgeBonusMult = 1.5f;
				Override.PunishAttackBonusMult = 1.5f;
				Override.CounterBonusMult = 1.75f;

				Brain->ApplyRuntimeOverride(Override);

				if (bDebugSpawner)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Spawner] %s spawned as ELITE"), *NewEnemy->GetName());
				}
			}
		}

		if (bDebugSpawner)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Spawner] Spawned %s with role %s at %s"),
				*NewEnemy->GetName(),
				*UEnum::GetValueAsString(RoleAsset->Role),
				*SpawnTransform.GetLocation().ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Spawner] Spawned actor has no EnemyBrainComponent."));
	}
}

void AWaveSpawnerActor::SpawnNextRoundRobin()
{
	if (RoleRotation.Num() == 0)
	{
		return;
	}

	SpawnEnemy(RoleRotation[RotationIndex]);
	RotationIndex = (RotationIndex + 1) % RoleRotation.Num();
}