#include "CombatComponent.h"
#include "HealthComponent.h"
#include "DefenseComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraCommon.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "HitReactionComponent.h"


UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    Defense = GetOwner()->FindComponentByClass<UDefenseComponent>();

    InitializeDecalPool();
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
        return;

    bIsAttacking = true;
    bCanAttack = false;

    CurrentAttackType = EAttackType::Light;
    CurrentAttackDamage = LightAttackDamage;

    ClearHitActors();

    OnAttackStarted.Broadcast();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combat] LIGHT STARTED | Damage=%.2f"
        ),
        CurrentAttackDamage
    );
}

void UCombatComponent::StartHeavyAttack()
{
    if (!bCanAttack)
        return;

    if (Defense && Defense->IsDodging())
        return;

    bIsAttacking = true;
    bCanAttack = false;

    CurrentAttackType = EAttackType::Heavy;
    CurrentAttackDamage = HeavyAttackDamage;

    ClearHitActors();

    OnAttackStarted.Broadcast();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combat] HEAVY STARTED | Damage=%.2f"
        ),
        CurrentAttackDamage
    );
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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Attack Ended")
        );
    }
}

/*=====================================================
                        WEAPON
=====================================================*/

void UCombatComponent::SetCurrentWeapon(AWeaponBase* NewWeapon)
{
    CurrentWeapon = NewWeapon;

    if (CurrentWeapon)
    {
        CurrentWeapon->SetOwningCombatComponent(this);
    }
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

void UCombatComponent::RegisterHit(
    AActor* HitActor,
    UPrimitiveComponent* HitComponent,
    const FVector& HitLocation,
    const FVector& HitNormal,
    const FVector& AttackDirection,
    FName HitBoneName)
{
    if (!HitActor)
        return;

    if (HitActor == GetOwner())
        return;

    // ---------------------------------------------------------
    // FRIENDLY FIRE
    // ---------------------------------------------------------

    APawn* AttackerPawn = Cast<APawn>(GetOwner());
    APawn* VictimPawn = Cast<APawn>(HitActor);

    if (AttackerPawn && VictimPawn)
    {
        const bool bAttackerIsPlayer =
            AttackerPawn->IsPlayerControlled();

        const bool bVictimIsPlayer =
            VictimPawn->IsPlayerControlled();

        if (bAttackerIsPlayer == bVictimIsPlayer)
        {
            return;
        }
    }

    // ---------------------------------------------------------
    // DEFENSE / INVULNERABILITY
    // ---------------------------------------------------------

    if (UDefenseComponent* VictimDefense =
        HitActor->FindComponentByClass<UDefenseComponent>())
    {
        if (VictimDefense->IsInvulnerable())
        {
            return;
        }
    }

    // ---------------------------------------------------------
    // HEALTH
    // ---------------------------------------------------------

    UHealthComponent* Health =
        HitActor->FindComponentByClass<UHealthComponent>();

    if (!Health)
        return;

    if (!Health->IsAlive())
        return;

    // ---------------------------------------------------------
    // ONE HIT PER ATTACK
    // ---------------------------------------------------------

    if (HasAlreadyHit(HitActor))
        return;

    HitActors.Add(HitActor);

    // ---------------------------------------------------------
    // HIT DIRECTION
    // ---------------------------------------------------------

    const FVector VictimForward =
        HitActor->GetActorForwardVector();

    const FVector ToAttacker =
        (
            GetOwner()->GetActorLocation() -
            HitActor->GetActorLocation()
            ).GetSafeNormal();

    const float ForwardDot =
        FVector::DotProduct(
            VictimForward,
            ToAttacker
        );

    const float RightDot =
        FVector::DotProduct(
            HitActor->GetActorRightVector(),
            ToAttacker
        );

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

    // ---------------------------------------------------------
    // DAMAGE
    // ---------------------------------------------------------

    float DamageToApply = CurrentAttackDamage;

    // Perfect dodge = DOUBLE the normal attack damage.
    //
    // Light: 20 -> 40
    // Heavy: 40 -> 80
    //
    // Time dilation has absolutely no effect on this calculation.

    if (bPerfectDodgeRewardWindowActive)
    {
        DamageToApply *= 2.0f;

        bPerfectDodgeRewardWindowActive = false;

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(
                PerfectDodgeRewardHandle
            );
        }

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[Combat] PERFECT DODGE HIT | "
                "Base=%.2f | Final=%.2f"
            ),
            CurrentAttackDamage,
            DamageToApply
        );
    }

    // ---------------------------------------------------------
    // DAMAGE
    // ---------------------------------------------------------

    if (DamageToApply <= 0.f)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[Combat] HIT REGISTERED BUT DAMAGE WAS 0 | "
                "AttackType=%d"
            ),
            static_cast<int32>(CurrentAttackType)
        );

        return;
    }

    Health->TakeDamage(
        DamageToApply,
        Direction
    );

    // ---------------------------------------------------------
    // HIT REACTION
    // ---------------------------------------------------------

    if (Health->IsAlive())
    {
        if (UHitReactionComponent* HitReaction =
            HitActor->FindComponentByClass<UHitReactionComponent>())
        {
            HitReaction->ReactToHit(Direction);
        }
    }

    // ---------------------------------------------------------
    // BLOOD DECAL
    // ---------------------------------------------------------

    SpawnBloodSpillDecals(
        HitLocation,
        HitNormal,
        HitActor
    );

    // ---------------------------------------------------------
    // BLOOD VFX
    // ---------------------------------------------------------

    if (BloodImpactEffect)
    {
        UNiagaraComponent* BloodFX =
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                BloodImpactEffect,
                HitLocation,
                FRotator::ZeroRotator,
                FVector(1.f),
                true,
                true,
                ENCPoolMethod::AutoRelease,
                true
            );

        if (BloodFX)
        {
            BloodFX->SetVariableVec3(
                TEXT("User.HitNormal"),
                HitNormal
            );

            BloodFX->SetVariableVec3(
                TEXT("User.AttackDirection"),
                AttackDirection
            );
        }
    }

    // ---------------------------------------------------------
    // SUCCESS
    // ---------------------------------------------------------

    OnSuccessfulHit.Broadcast(
        HitActor,
        HitLocation,
        HitNormal,
        AttackDirection
    );
}

void UCombatComponent::ClearHitActors()
{
    HitActors.Empty();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combat] HitActors Cleared")
    );
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

void UCombatComponent::SpawnBloodSpillDecals(
    const FVector& HitLocation,
    const FVector& HitNormal,
    AActor* Victim)
{
    if (!BloodDecalClass || !Victim)
        return;

    UWorld* World = GetWorld();

    if (!World)
        return;

    /*
        Anchor at the victim's FEET, not the hit height.
        A chest or head hit is too high above the floor for a
        short-range trace to reach it if we start from HitLocation.Z.
    */

    float FeetOffset = 90.f;

    if (ACharacter* VictimCharacter = Cast<ACharacter>(Victim))
    {
        if (UCapsuleComponent* Capsule = VictimCharacter->GetCapsuleComponent())
        {
            FeetOffset = Capsule->GetScaledCapsuleHalfHeight();
        }
    }

    const FVector ActorLocation = Victim->GetActorLocation();

    const FVector Origin =
        FVector(
            ActorLocation.X,
            ActorLocation.Y,
            ActorLocation.Z - FeetOffset + 15.f
        );

    TArray<FVector> Directions;

    Directions.Add(FVector(0.05f, 0.05f, -1.f).GetSafeNormal());
    Directions.Add(FVector(-0.15f, 0.1f, -1.f).GetSafeNormal());
    Directions.Add(FVector(0.15f, -0.15f, -1.f).GetSafeNormal());
    Directions.Add(FVector(0.6f, 0.1f, -0.35f).GetSafeNormal());
    Directions.Add(FVector(-0.6f, -0.1f, -0.35f).GetSafeNormal());

    const int32 NumSpills = FMath::RandRange(2, 3);
    int32 Spawned = 0;

    for (int32 i = Directions.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        Directions.Swap(i, j);
    }

    for (const FVector& Direction : Directions)
    {
        if (Spawned >= NumSpills)
            break;

        const float Distance = FMath::RandRange(8.f, 25.f);

        const FVector End = Origin + Direction * Distance;

        FHitResult SurfaceHit;

        FCollisionQueryParams Params(
            SCENE_QUERY_STAT(BloodSpillTrace),
            true
        );

        Params.AddIgnoredActor(GetOwner());
        Params.AddIgnoredActor(Victim);

        const bool bHit =
            World->LineTraceSingleByChannel(
                SurfaceHit,
                Origin,
                End,
                ECC_Visibility,
                Params
            );

        if (!bHit)
            continue;

        UPrimitiveComponent* SurfaceComponent = SurfaceHit.GetComponent();

        if (!SurfaceComponent)
            continue;

        if (SurfaceHit.GetActor() == Victim)
            continue;

        SpawnBloodDecalOnSurface(
            SurfaceHit,
            FMath::RandRange(1.2f, 2.3f)
        );

        Spawned++;
    }
}


void UCombatComponent::InitializeDecalPool()
{
    if (!BloodDecalClass)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    DecalPool.Reserve(DecalPoolSize);

    for (int32 i = 0; i < DecalPoolSize; ++i)
    {
        AActor* Decal = World->SpawnActor<AActor>(
            BloodDecalClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator
        );

        if (Decal)
        {
            Decal->SetActorHiddenInGame(true);
            Decal->SetActorEnableCollision(false);

            if (USceneComponent* Root = Decal->GetRootComponent())
            {
                Root->SetMobility(EComponentMobility::Movable);
            }

            DecalPool.Add(Decal);
        }

    }
}

AActor* UCombatComponent::GetPooledDecal()
{
    if (DecalPool.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[Combat] Decal pool EMPTY"));
        return nullptr;
    }

    AActor* Decal = DecalPool[DecalPoolIndex];
    DecalPoolIndex = (DecalPoolIndex + 1) % DecalPool.Num();

    UE_LOG(LogTemp, Warning, TEXT("[Combat] Returning pooled decal: %s"), *Decal->GetName());

    return Decal;
}

bool UCombatComponent::SpawnBloodDecalOnSurface(
    const FHitResult& SurfaceHit,
    float SizeMultiplier)
{
    if (!BloodDecalClass)
        return false;

    UWorld* World = GetWorld();

    if (!World)
        return false;

    FVector SurfaceNormal = SurfaceHit.ImpactNormal.GetSafeNormal();

    if (FVector::DotProduct(SurfaceNormal, FVector::UpVector) > 0.9f)
    {
        SurfaceNormal = FVector::UpVector;
    }

    const FVector ProjectionAxis = -SurfaceNormal;

    FVector UpReference = FVector::UpVector;

    if (FMath::Abs(FVector::DotProduct(ProjectionAxis, UpReference)) > 0.98f)
    {
        UpReference = FVector::ForwardVector;
    }

    const FVector RightAxis =
        FVector::CrossProduct(UpReference, ProjectionAxis).GetSafeNormal();

    const FVector UpAxis =
        FVector::CrossProduct(ProjectionAxis, RightAxis).GetSafeNormal();

    const FRotator DecalRotation =
        FMatrix(
            ProjectionAxis,
            RightAxis,
            UpAxis,
            FVector::ZeroVector
        ).Rotator();

    const FVector Location =
        SurfaceHit.ImpactPoint + SurfaceNormal * 1.5f;

    AActor* BloodDecal = GetPooledDecal();

    if (!BloodDecal)
        return false;

    BloodDecal->SetActorLocationAndRotation(Location, DecalRotation);
    BloodDecal->SetActorHiddenInGame(false);

    UDecalComponent* DecalComponent =
        BloodDecal->FindComponentByClass<UDecalComponent>();

    if (!DecalComponent)
    {
        BloodDecal->Destroy();
        return false;
    }
    DecalComponent->SetVisibility(true, true);
    DecalComponent->SetFadeIn(0.f, 0.f);

    const float RandomSize = FMath::RandRange(0.65f, 1.15f);
    const float AspectVariance = FMath::RandRange(0.7f, 1.3f);

    DecalComponent->DecalSize =
        FVector(
            8.f,
            45.f * RandomSize * SizeMultiplier,
            45.f * RandomSize * SizeMultiplier * AspectVariance
        );

    DecalComponent->SetFadeOut(45.f, 15.f, false);

    return true;
}

void UCombatComponent::ActivatePerfectDodgeRewardWindow(float Duration)
{
    bPerfectDodgeRewardWindowActive = true;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            PerfectDodgeRewardHandle, this, &UCombatComponent::ClearPerfectDodgeRewardWindow, Duration, false);
    }
}

void UCombatComponent::ClearPerfectDodgeRewardWindow()
{
    bPerfectDodgeRewardWindowActive = false;
}