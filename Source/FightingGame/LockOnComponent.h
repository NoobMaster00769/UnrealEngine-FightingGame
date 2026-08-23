#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.h"
#include "LockOnComponent.generated.h"

class USpringArmComponent;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API ULockOnComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockOnComponent();

    // --- State: BlueprintReadWrite so other BP graphs (HUD, anim BP, etc.) can still read/set these ---
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LockOn")
    bool bIsLockedOn = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "LockOn")
    AActor* CurrentTarget = nullptr;

    // --- Config, editable per-Blueprint-instance ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Config")
    float LockOnRange = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Config")
    float NormalSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Config")
    float CombatSpeed = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Config")
    TSubclassOf<AActor> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
    float ZoomInterpSpeed = 4.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
    float RotationInterpSpeed = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
    float NormalArmLength = 270.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
    float LockedArmLength = 170.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
    float DistanceRangeMin = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
    float DistanceRangeMax = 800.f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugLockOn = true;

    UPROPERTY(BlueprintAssignable, Category = "LockOn")
    FOnLockOnTargetChanged OnLockOnTargetChanged;

    /*==============================
            LOCK ON
    ==============================*/
    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestLockOn();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestUnlock();

    UFUNCTION(BlueprintPure, Category = "LockOn")
    bool IsLockedOn() const;

    UFUNCTION(BlueprintPure, Category = "LockOn")
    AActor* GetCurrentTarget() const;

    UFUNCTION()
    void HandleTargetDeath();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TArray<TWeakObjectPtr<AActor>> ViewConeEnemies;
    TWeakObjectPtr<USpringArmComponent> CameraBoom;
    TWeakObjectPtr<UCharacterMovementComponent> MovementComp;

    AActor* FindNearestEnemy() const;
    void CycleTarget();
    void SetLockedTarget(AActor* NewTarget);
    void UpdateCameraLock(float DeltaTime);
    float GetOwnerDistanceTo(const AActor* Other) const;
};