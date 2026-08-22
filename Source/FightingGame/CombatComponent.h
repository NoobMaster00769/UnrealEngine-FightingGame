#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NiagaraSystem.h"
#include "WeaponBase.h"
#include "CombatComponent.generated.h"

class UDefenseComponent;
class AActor;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None	UMETA(DisplayName = "None"),
	Light	UMETA(DisplayName = "Light"),
	Heavy	UMETA(DisplayName = "Heavy")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnSuccessfulHit,
	AActor*,
	HitActor,
	FVector,
	HitLocation,
	FVector,
	HitNormal,
	FVector,
	AttackDirection
);

UCLASS(ClassGroup = (Combat), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatComponent();

protected:

	virtual void BeginPlay() override;

public:

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/*=====================================================
						ATTACK
	=====================================================*/

	UFUNCTION(BlueprintPure)
	bool IsWeaponCollisionActive() const { return bWeaponCollisionActive; }

	UFUNCTION(BlueprintCallable)
	void StartLightAttack();

	UFUNCTION(BlueprintCallable)
	void StartHeavyAttack();

	UFUNCTION(BlueprintCallable)
	void EndAttack();

	/*=====================================================
						WEAPON
	=====================================================*/

	UFUNCTION(BlueprintCallable)
	void SetCurrentWeapon(AWeaponBase* NewWeapon);

	UFUNCTION(BlueprintPure)
	AWeaponBase* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable)
	void EnableWeaponCollision();

	UFUNCTION(BlueprintCallable)
	void DisableWeaponCollision();

	/*=====================================================
					  HIT REGISTRATION
	=====================================================*/


	UFUNCTION()
	void SpawnBloodSpillDecals(
		const FVector& HitLocation,
		const FVector& HitNormal,
		AActor* Victim
	);

	UFUNCTION(BlueprintCallable)
	void RegisterHit(
		AActor* HitActor,
		UPrimitiveComponent* HitComponent,
		const FVector& HitLocation,
		const FVector& HitNormal,
		const FVector& AttackDirection,
		FName HitBoneName = NAME_None
	);

	UFUNCTION(BlueprintCallable)
	void ClearHitActors();

	UFUNCTION(BlueprintPure)
	bool HasAlreadyHit(AActor* HitActor) const;

	/*=====================================================
						GETTERS
	=====================================================*/

	UFUNCTION(BlueprintPure)
	bool IsAttacking() const;

	UFUNCTION(BlueprintPure)
	bool CanAttack() const;

	UFUNCTION(BlueprintPure)
	float GetCurrentDamage() const;

	UFUNCTION(BlueprintPure)
	EAttackType GetAttackType() const;

	/*=====================================================
						EVENTS
	=====================================================*/

	UPROPERTY(BlueprintAssignable)
	FOnAttackStarted OnAttackStarted;

	UPROPERTY(BlueprintAssignable)
	FOnAttackEnded OnAttackEnded;

	UPROPERTY(BlueprintAssignable)
	FOnSuccessfulHit OnSuccessfulHit;

	UFUNCTION(BlueprintCallable)
	void SetCanAttack(bool bNewCanAttack);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Blood")
	TSubclassOf<AActor> BloodDecalClass;

	UFUNCTION(BlueprintCallable)
	void SetPerfectDodgeWindowOpen(bool bOpen) { bPerfectDodgeWindowOpen = bOpen; }

	UFUNCTION(BlueprintPure)
	bool IsPerfectDodgeWindowOpen() const { return bPerfectDodgeWindowOpen; }

	UFUNCTION(BlueprintCallable)
	void ActivatePerfectDodgeRewardWindow(float Duration);

	UFUNCTION(BlueprintPure)
	bool IsPerfectDodgeRewardWindowActive() const { return bPerfectDodgeRewardWindowActive; }

	UPROPERTY(EditAnywhere, Category = "Combat|PerfectDodge")
	float PerfectDodgeDamageMultiplier = 2.5f;

private:
	bool bPerfectDodgeWindowOpen = false;   // set by AnimNotifyState, true while the notify is active
	bool bPerfectDodgeRewardWindowActive = false;
	FTimerHandle PerfectDodgeRewardHandle;

	void ClearPerfectDodgeRewardWindow();

private:

	bool SpawnBloodDecalOnSurface(
		const FHitResult& SurfaceHit,
		float SizeMultiplier = 1.0f
	);

	// --- Decal pooling ---
	void InitializeDecalPool();
	AActor* GetPooledDecal();

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blood")
	int32 DecalPoolSize = 20;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> DecalPool;

	int32 DecalPoolIndex = 0;

	UPROPERTY()
	UDefenseComponent* Defense = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bWeaponCollisionActive = false;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
	float LightAttackDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
	float HeavyAttackDamage = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bDebugCombat = true;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bCanAttack = true;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	EAttackType CurrentAttackType = EAttackType::None;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	float CurrentAttackDamage = 0.f;

	UPROPERTY()
	AWeaponBase* CurrentWeapon = nullptr;


	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> BloodImpactEffect;
};