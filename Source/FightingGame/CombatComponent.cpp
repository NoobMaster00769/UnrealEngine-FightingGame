#include "CombatComponent.h"
#include "HealthComponent.h"
#include "DefenseComponent.h"
#include "HitReactionComponent.h"


UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    Defense = GetOwner()->FindComponentByClass<UDefenseComponent>();
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

    if (Defense && Defense->IsDodging())
    {
        return;
    }

    bIsAttacking = true;
    bCanAttack = false;

    CurrentAttackType = EAttackType::Light;
    CurrentAttackDamage = LightAttackDamage;

    ClearHitActors();

    OnAttackStarted.Broadcast();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combat] Broadcast Attack Started (Light)")
    );

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

    if (Defense && Defense->IsDodging())
    {
        return;
    }
    bIsAttacking = true;
    bCanAttack = false;

    CurrentAttackType = EAttackType::Heavy;
    CurrentAttackDamage = HeavyAttackDamage;

    ClearHitActors();

    OnAttackStarted.Broadcast();
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combat] Broadcast Attack Started (Heavy)")
    );
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

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            5.f,
            FColor::Green,
            TEXT("EndAttack Called"));
    }

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
    bWeaponCollisionActive = true;

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
    bWeaponCollisionActive = false;

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
        const FVector VictimForward =
            HitActor->GetActorForwardVector();

        const FVector ToAttacker =
            (GetOwner()->GetActorLocation() -
                HitActor->GetActorLocation()).GetSafeNormal();

        const float ForwardDot =
            FVector::DotProduct(
                VictimForward,
                ToAttacker);

        const float RightDot =
            FVector::DotProduct(
                HitActor->GetActorRightVector(),
                ToAttacker);

        EHitDirection Direction;

        if (ForwardDot > 0.7f)
        {
            Direction = EHitDirection::Front;
        }
        else if (ForwardDot < -0.7f)
        {
            Direction = EHitDirection::Back;
        }
        else if (RightDot > 0.f)
        {
            Direction = EHitDirection::Right;
        }
        else
        {
            Direction = EHitDirection::Left;
        }

        Health->TakeDamage(
            CurrentAttackDamage,
            Direction);

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

void UCombatComponent::SetCanAttack(bool bNewCanAttack)
{
    bCanAttack = bNewCanAttack;
}