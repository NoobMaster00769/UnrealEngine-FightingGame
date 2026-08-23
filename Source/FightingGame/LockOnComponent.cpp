#include "LockOnComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"

ULockOnComponent::ULockOnComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::BeginPlay()
{
    Super::BeginPlay();

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    MovementComp = OwnerChar->GetCharacterMovement();
    CameraBoom = OwnerChar->FindComponentByClass<USpringArmComponent>();
}

float ULockOnComponent::GetOwnerDistanceTo(const AActor* Other) const
{
    if (!GetOwner() || !Other) return TNumericLimits<float>::Max();
    return GetOwner()->GetDistanceTo(Other);
}

void ULockOnComponent::RequestLockOn()
{
    if (!bIsLockedOn)
    {
        AActor* Nearest = FindNearestEnemy();
        if (Nearest)
        {
            bIsLockedOn = true;
            SetLockedTarget(Nearest);
            if (MovementComp.IsValid())
                MovementComp->MaxWalkSpeed = CombatSpeed;

            if (bDebugLockOn && GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
                    FString::Printf(TEXT("Locked: %s"), *Nearest->GetName()));
        }
        else if (bDebugLockOn && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No enemy in range"));
        }
    }
    else
    {
        CycleTarget();
    }
}

void ULockOnComponent::RequestUnlock()
{
    if (bIsLockedOn)
    {
        bIsLockedOn = false;
        if (MovementComp.IsValid())
            MovementComp->MaxWalkSpeed = NormalSpeed;
        SetLockedTarget(nullptr);

        if (bDebugLockOn && GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Unlocked"));
    }
}

bool ULockOnComponent::IsLockedOn() const
{
    return bIsLockedOn;
}

AActor* ULockOnComponent::GetCurrentTarget() const
{
    return CurrentTarget;
}

void ULockOnComponent::SetLockedTarget(AActor* NewTarget)
{
    CurrentTarget = NewTarget;
    OnLockOnTargetChanged.Broadcast(NewTarget);

    if (NewTarget)
    {
        if (UHealthComponent* HC = NewTarget->FindComponentByClass<UHealthComponent>())
        {
            HC->OnDeath.AddDynamic(this, &ULockOnComponent::HandleTargetDeath);
        }
    }
}


void ULockOnComponent::HandleTargetDeath()
{
    RequestUnlock();
}

AActor* ULockOnComponent::FindNearestEnemy() const
{
    if (!GetWorld() || !EnemyClass) return nullptr;

    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), EnemyClass, AllEnemies);

    AActor* Best = nullptr;
    float BestDist = TNumericLimits<float>::Max();

    for (AActor* Enemy : AllEnemies)
    {
        if (!IsValid(Enemy)) continue;
        const float Dist = GetOwnerDistanceTo(Enemy);
        if (Dist < LockOnRange && Dist < BestDist)
        {
            BestDist = Dist;
            Best = Enemy;
        }
    }
    return Best;
}

void ULockOnComponent::CycleTarget()
{
    if (!GetWorld() || !EnemyClass) return;

    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), EnemyClass, AllEnemies);

    ViewConeEnemies.Empty();
    for (AActor* Enemy : AllEnemies)
    {
        if (IsValid(Enemy) && GetOwnerDistanceTo(Enemy) < LockOnRange)
            ViewConeEnemies.Add(Enemy);
    }

    if (ViewConeEnemies.Num() == 0)
    {
        RequestUnlock();
        return;
    }

    int32 CurrentIndex = ViewConeEnemies.IndexOfByPredicate(
        [this](const TWeakObjectPtr<AActor>& E) { return E.Get() == CurrentTarget; });

    if (CurrentIndex == INDEX_NONE)
    {
        AActor* Nearest = FindNearestEnemy();
        if (Nearest) SetLockedTarget(Nearest);
        else RequestUnlock();
        return;
    }

    int32 NextIndex = (CurrentIndex + 1) % ViewConeEnemies.Num();
    SetLockedTarget(ViewConeEnemies[NextIndex].Get());

    if (bDebugLockOn && GEngine && ViewConeEnemies[NextIndex].IsValid())
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Cycled: %s"), *ViewConeEnemies[NextIndex]->GetName()));
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsLockedOn && !IsValid(CurrentTarget))
    {
        RequestUnlock();
        return;
    }

    if (bIsLockedOn && IsValid(CurrentTarget))
    {
        UpdateCameraLock(DeltaTime);
    }
}

void ULockOnComponent::UpdateCameraLock(float DeltaTime)
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar || !CurrentTarget) return;

    if (AController* Ctrl = OwnerChar->GetController())
    {
        FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(
            OwnerChar->GetActorLocation(), CurrentTarget->GetActorLocation());
        FRotator NewRot = FMath::RInterpTo(Ctrl->GetControlRotation(), LookAt, DeltaTime, RotationInterpSpeed);
        Ctrl->SetControlRotation(NewRot);
    }

    if (CameraBoom.IsValid())
    {
        float Dist = GetOwnerDistanceTo(CurrentTarget);
        float TargetArmLength = FMath::GetMappedRangeValueClamped(
            FVector2D(DistanceRangeMin, DistanceRangeMax),
            FVector2D(LockedArmLength, NormalArmLength),
            Dist);
        CameraBoom->TargetArmLength = FMath::FInterpTo(
            CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
    }
}