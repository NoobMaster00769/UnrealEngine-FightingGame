#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DefenseComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCombatComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerfectDodge, AActor*, ThreatActor);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UDefenseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDefenseComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable)
    bool StartDodge(const FVector& Direction, AActor* IncomingThreatActor = nullptr);

    UFUNCTION(BlueprintCallable)
    void EndDodge();

    void FinishAttackLock();

    UFUNCTION(BlueprintPure)
    bool IsDodging() const { return bIsDodging; }

    bool IsInvulnerable() const { return bInvulnerable; }

    UFUNCTION(BlueprintPure)
    bool CanDodge() const { return bCanDodge; }

    UFUNCTION(BlueprintPure)
    FVector GetDodgeDirection() const { return DodgeDirection; }

    UFUNCTION(BlueprintPure)
    bool WasLastDodgePerfect() const { return bLastDodgeWasPerfect; }

public:
    UPROPERTY(BlueprintAssignable)
    FOnDodgeStarted OnDodgeStarted;

    UPROPERTY(BlueprintAssignable)
    FOnDodgeEnded OnDodgeEnded;

    UPROPERTY(BlueprintAssignable)
    FOnPerfectDodge OnPerfectDodge;

public:
    UPROPERTY(EditAnywhere, Category = "Dodge")
    float DodgeDistance = 350.f;

    UPROPERTY(EditAnywhere, Category = "Dodge")
    float DodgeDuration = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Dodge")
    float DodgeCooldown = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Dodge")
    float AttackLockTime = 1.5f;

private:
    void UpdateDodge(float DeltaTime);
    void StartCooldown();
    void FinishCooldown();

private:
    UPROPERTY()
    ACharacter* OwnerCharacter = nullptr;

    UPROPERTY()
    UCharacterMovementComponent* Movement = nullptr;

private:
    FRotator LockedRotation;
    bool bIsDodging = false;
    bool bCanDodge = true;
    bool bInvulnerable = false;
    bool bLastDodgeWasPerfect = false;
    FVector DodgeDirection = FVector::ZeroVector;
    float ElapsedTime = 0.f;
    float CurrentSpeed = 0.f;
    FTimerHandle CooldownHandle;
    FTimerHandle AttackLockHandle;
};