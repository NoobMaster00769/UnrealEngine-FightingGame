#include "EnemyMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "DefenseComponent.h"
#include "NavigationPath.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "GameFramework/Character.h"

UEnemyMovementComponent::UEnemyMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UEnemyMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    bStrafeLeft = FMath::RandBool();
}

void UEnemyMovementComponent::TickComponent(


    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (!OwnerCharacter)
    {
        return;
    }

    UDefenseComponent* Defense =
        OwnerCharacter->FindComponentByClass<UDefenseComponent>();

    if (Defense && Defense->IsDodging())
    {
        return;
    }

    if (!CurrentTarget)
    {
        return;
    }

    //---------------------------------
    // Face player
    //---------------------------------

    FVector ToPlayer =
        CurrentTarget->GetActorLocation() -
        OwnerCharacter->GetActorLocation();

    ToPlayer.Z = 0.f;

    if (!ToPlayer.IsNearlyZero())
    {
        FRotator Desired =
            ToPlayer.Rotation();

        FRotator NewRotation =
            FMath::RInterpTo(
                OwnerCharacter->GetActorRotation(),
                Desired,
                DeltaTime,
                RotationInterpSpeed);

        OwnerCharacter->SetActorRotation(NewRotation);
    }

    //---------------------------------
    // Movement
    //---------------------------------

    FVector Forward =
        OwnerCharacter->GetActorForwardVector();

    FVector Right =
        OwnerCharacter->GetActorRightVector();

    switch (CurrentMovementMode)
    {
    case EEnemyMovementMode::Approach:

        OwnerCharacter->AddMovementInput(
            Forward,
            MovementSpeed);

        break;

    case EEnemyMovementMode::Retreat:

        OwnerCharacter->AddMovementInput(
            Forward,
            -MovementSpeed);

        break;

    case EEnemyMovementMode::Strafe:

        OwnerCharacter->AddMovementInput(
            Right,
            bStrafeLeft ? -MovementSpeed : MovementSpeed);

        break;

    default:
        break;
    }
}


void UEnemyMovementComponent::ApproachTarget(AActor* Target)
{
    if (!OwnerCharacter || !Target)
        return;

    CurrentTarget = Target;
    CurrentMovementMode = EEnemyMovementMode::Approach;
}



void UEnemyMovementComponent::RetreatFromTarget(AActor* Target)
{
    if (!OwnerCharacter || !Target)
        return;

    CurrentTarget = Target;
    CurrentMovementMode = EEnemyMovementMode::Retreat;
}

void UEnemyMovementComponent::StrafeAroundTarget(AActor* Target)
{
    if (!OwnerCharacter || !Target)
        return;

    CurrentTarget = Target;
    CurrentMovementMode = EEnemyMovementMode::Strafe;
}

void UEnemyMovementComponent::StopMovement()
{
    CurrentMovementMode = EEnemyMovementMode::None;
}

void UEnemyMovementComponent::SetTarget(AActor* Target)
{
    CurrentTarget = Target;
}

