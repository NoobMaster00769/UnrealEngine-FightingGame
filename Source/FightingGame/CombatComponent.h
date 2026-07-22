#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponBase.h"
#include "CombatComponent.generated.h"


UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None	UMETA(DisplayName = "None"),
	Light	UMETA(DisplayName = "Light"),
	Heavy	UMETA(DisplayName = "Heavy")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSuccessfulHit, AActor*, HitActor);

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

	UFUNCTION(BlueprintCallable)
	void RegisterHit(AActor* HitActor);

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

private:

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

	/*
		TSet gives O(1) lookup instead of
		searching through an array every swing.
	*/

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;
};