#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyRoleDataAsset.h"
#include "CombatDirectorSubsystem.generated.h"

class UCombatComponent;
class AActor;

USTRUCT(BlueprintType)
struct FPlayerCombatStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float DamageDealtEstimate = 0.f;   // decaying rate, not lifetime total

	UPROPERTY(BlueprintReadOnly)
	float DamageTakenEstimate = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Accuracy = 0.5f;             // hits landed / swings attempted, EWMA

	UPROPERTY(BlueprintReadOnly)
	float AverageComboLength = 1.f;

	UPROPERTY(BlueprintReadOnly)
	float PreferredDistance = 250.f;   // EWMA of sampled distance while engaged

	UPROPERTY(BlueprintReadOnly)
	float GlobalLeftDodgeBias = 0.5f;
};

UCLASS()
class FIGHTINGGAME_API UCombatDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Called once by the player's CombatComponent when it's set up (mirrors
	// how EnemyBrainComponent already resolves sibling components).
	UFUNCTION(BlueprintCallable)
	void RegisterPlayerCombat(AActor* PlayerActor, UCombatComponent* PlayerCombat);

	// Called once per enemy that engages the player, so distance sampling
	// and combo-length observation has something concrete to watch.
	UFUNCTION(BlueprintCallable)
	void RegisterActiveEnemy(AActor* EnemyActor);

	UFUNCTION(BlueprintCallable)
	void UnregisterActiveEnemy(AActor* EnemyActor);

	UFUNCTION(BlueprintPure)
	const FPlayerCombatStats& GetPlayerStats() const { return Stats; }

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugDirector = true;

	UFUNCTION(BlueprintCallable)
	bool TrySpendBudget(EEnemyRole Role);

	UFUNCTION(BlueprintPure)
	EEnemyRole ChooseNextRole() const;

	UFUNCTION(BlueprintPure)
	float GetCurrentBudget() const { return CurrentBudget; }

	UPROPERTY(EditAnywhere, Category = "Director")
	float BudgetGrowthPerSecond = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Director")
	float MaxBudget = 20.f;

	UFUNCTION(BlueprintCallable)
	void ReportDodgeBiasObservation(float LeftBias);

	UPROPERTY(EditAnywhere, Category = "Director")
	float TensionPeriodSeconds = 45.f;

	UPROPERTY(EditAnywhere, Category = "Director")
	float TensionAmplitude = 0.3f;

	UFUNCTION(BlueprintCallable)
	bool TryAcquireAttackToken(AActor* Enemy);

	UFUNCTION(BlueprintCallable)
	void ReleaseAttackToken(AActor* Enemy);

	UPROPERTY(EditAnywhere, Category = "Director")
	int32 MaxConcurrentAttackers = 2;

private:
	TArray<AActor*> ActiveAttackers;

private:
	float GetRoleCost(EEnemyRole Role) const;
	void TickBudget();

	float CurrentBudget = 3.f;
	FTimerHandle BudgetTimer;
	float BudgetTickInterval = 1.f;
private:
	UFUNCTION()
	void HandlePlayerSuccessfulHit(AActor* HitActor);

	UFUNCTION()
	void HandleEnemySuccessfulHit(AActor* HitActor);

	void SampleDistances();

	UPROPERTY()
	AActor* PlayerActor = nullptr;

	UPROPERTY()
	UCombatComponent* PlayerCombat = nullptr;

	UPROPERTY()
	TArray<AActor*> ActiveEnemies;

	UFUNCTION()
	void HandlePlayerAttackStarted();

	FPlayerCombatStats Stats;

	float AttacksAttempted = 0.f;
	float AttacksLanded = 0.f;

	FTimerHandle SampleTimer;
	float SampleInterval = 0.5f;

	float HitStatsDecayAlpha = 0.4f;   // damage dealt/taken - big steps, rare events

	float StatsDecayRate = 0.1f;

	float ElapsedTime = 0.f;
};

