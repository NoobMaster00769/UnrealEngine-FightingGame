#include "DefenseComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CombatComponent.h"

UDefenseComponent::UDefenseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UDefenseComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACharacter>(GetOwner());

    if (OwnerCharacter)
    {
        Movement = OwnerCharacter->GetCharacterMovement();
    }
}

void UDefenseComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (bIsDodging)
    {
        UpdateDodge(DeltaTime);
    }
}

bool UDefenseComponent::StartDodge(const FVector& Direction)
{
    if (!OwnerCharacter)
    {
        return false;
    }

    if (!Movement)
    {
        return false;
    }

    if (!bCanDodge)
    {
        return false;
    }

    if (bIsDodging)
    {
        return false;
    }

    FVector FinalDirection = Direction;

    if (FinalDirection.IsNearlyZero())
    {
        FinalDirection = OwnerCharacter->GetActorForwardVector();
    }

    FinalDirection.Normalize();

    DodgeDirection = FinalDirection;

    LockedRotation = FinalDirection.Rotation();
    LockedRotation.Pitch = 0.f;
    LockedRotation.Roll = 0.f;

    OwnerCharacter->SetActorRotation(LockedRotation);

    Movement->StopMovementImmediately();
    Movement->DisableMovement();

    CurrentSpeed = DodgeDistance / DodgeDuration;

    const FVector HorizontalDirection =
        FVector(DodgeDirection.X, DodgeDirection.Y, 0.f).GetSafeNormal();

    Movement->Velocity =
        HorizontalDirection * CurrentSpeed;

    ElapsedTime = 0.f;

    bIsDodging = true;
    bCanDodge = false;
    bInvulnerable = true;

    if (UCombatComponent* Combat = OwnerCharacter->FindComponentByClass<UCombatComponent>())
    {
        Combat->SetCanAttack(false);
    }

    OnDodgeStarted.Broadcast();

    return true;
}

void UDefenseComponent::UpdateDodge(float DeltaTime)
{   
    if (!Movement)
    {
        EndDodge();
        return;
    }

    ElapsedTime += DeltaTime;

    const float Alpha = FMath::Clamp(
        ElapsedTime / DodgeDuration,
        0.f,
        1.f);

    // Smooth ease-out
    const float SpeedMultiplier =
        FMath::InterpEaseOut(1.f, 0.f, Alpha, 3.5f);

    const float Speed =
        CurrentSpeed * SpeedMultiplier;

    const FVector HorizontalDirection =
        FVector(DodgeDirection.X, DodgeDirection.Y, 0.f).GetSafeNormal();

    OwnerCharacter->AddActorWorldOffset(
        HorizontalDirection * Speed * DeltaTime,
        true);

    OwnerCharacter->SetActorRotation(
        FRotator(
            0.f,
            LockedRotation.Yaw,
            0.f));

    if (ElapsedTime >= DodgeDuration)
    {
        EndDodge();
    }
}

void UDefenseComponent::EndDodge()
{
    if (!bIsDodging)
    {
        return;
    }
    if (Movement)
    {
        Movement->SetMovementMode(MOVE_Walking);
        Movement->StopMovementImmediately();
    }

    bIsDodging = false;
    bInvulnerable = false;

    StartCooldown();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            AttackLockHandle,
            this,
            &UDefenseComponent::FinishAttackLock,
            AttackLockTime,
            false);
    }

    OnDodgeEnded.Broadcast();
}

void UDefenseComponent::StartCooldown()
{
    if (!GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        CooldownHandle,
        this,
        &UDefenseComponent::FinishCooldown,
        DodgeCooldown,
        false);
}

void UDefenseComponent::FinishCooldown()
{
    bCanDodge = true;
}

void UDefenseComponent::FinishAttackLock()
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (UCombatComponent* Combat = OwnerCharacter->FindComponentByClass<UCombatComponent>())
    {
        Combat->SetCanAttack(true);
    }
}