#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h" 
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameplayActivated);

UCLASS()
class FIGHTINGGAME_API UCombatDirectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay State")
	bool bGameplayActive = false;

	UPROPERTY(BlueprintAssignable, Category = "Gameplay State")
	FOnGameplayActivated OnGameplayActivated;

	UFUNCTION(BlueprintCallable, Category = "Gameplay State")
	void SetGameplayActive(bool bNewState)
	{
		if (bGameplayActive == bNewState) return;
		bGameplayActive = bNewState;
		if (bGameplayActive)
		{
			OnGameplayActivated.Broadcast();
		}
	}

	UFUNCTION(BlueprintPure, Category = "Gameplay State")
	bool IsGameplayActive() const { return bGameplayActive; }
public:

	UFUNCTION(BlueprintPure)
	AActor* FindAttackerWithOpenPerfectDodgeWindow(AActor* Requester, float SearchRadius) const;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable)
	void RegisterPlayerCombat(AActor* PlayerActor, UCombatComponent* PlayerCombat);

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

	UFUNCTION(BlueprintPure)
	float GetFlankSlotAngle(AActor* Enemy) const;

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
	void HandlePlayerSuccessfulHit(
		AActor* HitActor,
		FVector HitLocation,
		FVector HitNormal,
		FVector AttackDirection
	);

	UFUNCTION()
	void HandleEnemySuccessfulHit(
		AActor* HitActor,
		FVector HitLocation,
		FVector HitNormal,
		FVector AttackDirection
	);

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

