#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthChanged);

UCLASS(ClassGroup = (Combat), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UHealthComponent();

protected:

    virtual void BeginPlay() override;

public:

    /*==============================
            HEALTH
    ==============================*/

    UFUNCTION(BlueprintCallable)
    void InitializeHealth(float StartingHealth);

    UFUNCTION(BlueprintCallable)
    void TakeDamage(float Damage);

    UFUNCTION(BlueprintCallable)
    void Heal(float Amount);

    UFUNCTION(BlueprintPure)
    float GetCurrentHealth() const;

    UFUNCTION(BlueprintPure)
    float GetMaxHealth() const;

    UFUNCTION(BlueprintPure)
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure)
    bool IsAlive() const;

    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable)
    FOnDeath OnDeath;

private:

    UPROPERTY(VisibleAnywhere, Category = "Health")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "Health")
    float CurrentHealth = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "Health")
    bool bIsDead = false;

    UPROPERTY(EditAnywhere, Category = "Health")
    bool bCanTakeDamage = true;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugHealth = true;
};