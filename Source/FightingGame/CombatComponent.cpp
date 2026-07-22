#include "CombatComponent.h"
#include "HealthComponent.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UCombatComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);
}

/*=====================================================
                        ATTACK
=====================================================*/

void UCombatComponent::StartLightAttack()
{
    if (!bCanAttack)
        return;

    bIsAttacking = true;
    bCanAttack = false;

    CurrentAttackType = EAttackType::Light;
    CurrentAttackDamage = LightAttackDamage;

    ClearHitActors();

    OnAttackStarted.Broadcast();

    if (bDebugCombat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Light Attack Started"));
    }
}

void UCombatComponent::StartHeavyAttack()
{
    if (!bCanAttack)
        return;

    bIsAttacking = true;
    bCanAttack = false;

    CurrentAttackType = EAttackType::Heavy;
    CurrentAttackDamage = HeavyAttackDamage;

    ClearHitActors();

    OnAttackStarted.Broadcast();

    if (bDebugCombat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Heavy Attack Started"));
    }
}

void UCombatComponent::EndAttack()
{
    bIsAttacking = false;
    bCanAttack = true;

    CurrentAttackType = EAttackType::None;
    CurrentAttackDamage = 0.f;

    DisableWeaponCollision();

    OnAttackEnded.Broadcast();

    if (bDebugCombat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Attack Ended"));
    }
}

/*=====================================================
                        WEAPON
=====================================================*/

void UCombatComponent::SetCurrentWeapon(AWeaponBase* NewWeapon)
{
    CurrentWeapon = NewWeapon;
}

AWeaponBase* UCombatComponent::GetCurrentWeapon() const
{
    return CurrentWeapon;
}

void UCombatComponent::EnableWeaponCollision()
{
    if (!CurrentWeapon)
        return;

    CurrentWeapon->EnableCollision();

    if (bDebugCombat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Weapon Collision Enabled"));
    }
}

void UCombatComponent::DisableWeaponCollision()
{
    if (!CurrentWeapon)
        return;

    CurrentWeapon->DisableCollision();

    if (bDebugCombat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Weapon Collision Disabled"));
    }
}

/*=====================================================
                    HIT REGISTRATION
=====================================================*/

void UCombatComponent::RegisterHit(AActor* HitActor)
{
    if (!HitActor)
        return;

    if (HitActor == GetOwner())
        return;

    if (HasAlreadyHit(HitActor))
        return;

    HitActors.Add(HitActor);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.f,
            FColor::Green,
            FString::Printf(TEXT("RegisterHit -> %s"), *HitActor->GetName()));
    }

    if (UHealthComponent* Health =
        HitActor->FindComponentByClass<UHealthComponent>())
    {
        Health->TakeDamage(CurrentAttackDamage);

        if (bDebugCombat)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Applied %.1f damage to %s"),
                CurrentAttackDamage,
                *HitActor->GetName());
        }
    }

    OnSuccessfulHit.Broadcast(HitActor);

    if (bDebugCombat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Registered Hit : %s"),
            *HitActor->GetName());
    }
}

void UCombatComponent::ClearHitActors()
{
    HitActors.Empty();
}

bool UCombatComponent::HasAlreadyHit(AActor* HitActor) const
{
    return HitActors.Contains(HitActor);
}

/*=====================================================
                        GETTERS
=====================================================*/

bool UCombatComponent::IsAttacking() const
{
    return bIsAttacking;
}

bool UCombatComponent::CanAttack() const
{
    return bCanAttack;
}

float UCombatComponent::GetCurrentDamage() const
{
    return CurrentAttackDamage;
}

EAttackType UCombatComponent::GetAttackType() const
{
    return CurrentAttackType;
}