#include "HealthComponent.h"

#include "Engine/Engine.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UHealthComponent::InitializeHealth(float StartingHealth)
{
    MaxHealth = StartingHealth;
    CurrentHealth = StartingHealth;

    bIsDead = false;
    bCanTakeDamage = true;

    if (bDebugHealth && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Green,
            FString::Printf(TEXT("Initialized Health: %.0f"), CurrentHealth));
    }
}
void UHealthComponent::TakeDamage(float Damage)
{
    if (!bCanTakeDamage || bIsDead)
        return;

    CurrentHealth = FMath::Clamp(
        CurrentHealth - Damage,
        0.f,
        MaxHealth);

    if (bDebugHealth && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Red,
            FString::Printf(
                TEXT("Health = %.1f / %.1f"),
                CurrentHealth,
                MaxHealth));
    }

    OnHealthChanged.Broadcast();

    if (CurrentHealth <= 0.f)
    {
        bIsDead = true;

        if (bDebugHealth && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.f,
                FColor::Yellow,
                TEXT("DEAD"));
        }

        OnDeath.Broadcast();
    }
}
void UHealthComponent::Heal(float Amount)
{
    if (bIsDead)
        return;

    CurrentHealth = FMath::Clamp(
        CurrentHealth + Amount,
        0.f,
        MaxHealth);

    OnHealthChanged.Broadcast();

    if (bDebugHealth && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Green,
            FString::Printf(
                TEXT("Healed -> %.1f / %.1f"),
                CurrentHealth,
                MaxHealth));
    }
}

float UHealthComponent::GetCurrentHealth() const
{
    return CurrentHealth;
}

float UHealthComponent::GetMaxHealth() const
{
    return MaxHealth;
}

float UHealthComponent::GetHealthPercent() const
{
    if (MaxHealth <= 0.f)
    {
        return 0.f;
    }

    return CurrentHealth / MaxHealth;
}

bool UHealthComponent::IsAlive() const
{
    return !bIsDead;
}