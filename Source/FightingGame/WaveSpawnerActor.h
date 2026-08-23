#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyRoleDataAsset.h"
#include "WaveSpawnerActor.generated.h"

class UEnemyBrainComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWaveStarted,
	int32,
	WaveNumber
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnEnemyKilled,
	int32,
	TotalKills
);

UCLASS()
class FIGHTINGGAME_API AWaveSpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	AWaveSpawnerActor();


public:
	UFUNCTION(BlueprintCallable)
	void SpawnEnemy(UEnemyRoleDataAsset* RoleAsset);

	UFUNCTION(BlueprintCallable)
	void SpawnNextRoundRobin();

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<APawn> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TObjectPtr<UEnemyRoleDataAsset>> RoleRotation;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugSpawner = true;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float EliteChance = 0.15f;

	UFUNCTION(BlueprintCallable)
	void TryDirectorSpawn();

	UPROPERTY(BlueprintAssignable, Category = "Waves")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Waves")
	FOnEnemyKilled OnEnemyKilled;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 EnemiesKilled = 0;

	UPROPERTY(EditAnywhere, Category = "Waves")
	int32 MaxEnemiesPerWave = 4;

	UPROPERTY(EditAnywhere, Category = "Waves")
	float TimeBetweenWaves = 4.f;

	UPROPERTY(BlueprintReadOnly, Category = "Waves")
	int32 CurrentWaveNumber = 0;

	UFUNCTION(BlueprintCallable)
	void StartNextWave();

	UFUNCTION()
	void HandleEnemyDeath();


private:
	int32 AliveCount = 0;
	bool bWaveInProgress = false;
	FTimerHandle NextWaveTimer;

private:
	UEnemyRoleDataAsset* FindRoleAsset(EEnemyRole InRole) const;

private:
	AActor* GetRandomSpawnPoint() const;

	int32 RotationIndex = 0;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnGameplayActivatedHandler();
};