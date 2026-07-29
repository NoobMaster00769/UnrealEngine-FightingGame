#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "EnemyRoleDataAsset.h"
#include "EnemyBrainComponent.generated.h"
class UCombatComponent;
class UDefenseComponent;
class UHealthComponent;
class UEnemyMovementComponent;

class AActor;


UENUM(BlueprintType)
enum class ECombatAction : uint8
{
    None,
    Approach,
    Retreat,
    Strafe,
    LightAttack,
    HeavyAttack,
    Dodge,
    Wait
};

USTRUCT(BlueprintType)
struct FEnemyBrainContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToTarget = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float EnemyHealthPercent = 1.f;

    UPROPERTY(BlueprintReadOnly)
    float TargetHealthPercent = 1.f;

    UPROPERTY(BlueprintReadOnly)
    bool bCanAttack = true;

    UPROPERTY(BlueprintReadOnly)
    bool bCanMove = true;

    UPROPERTY(BlueprintReadOnly)
    bool bCanDodge = true;
};

USTRUCT(BlueprintType)
struct FDecisionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ECombatAction Action = ECombatAction::Wait;

    UPROPERTY(BlueprintReadOnly)
    float Score = -100000.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDecisionMade, ECombatAction, Action);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UEnemyBrainComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UEnemyBrainComponent();

protected:

    virtual void BeginPlay() override;
    const FRoleProfile& GetProfile() const;

public:

    UFUNCTION(BlueprintCallable)
    void InitializeBrain();

    UFUNCTION(BlueprintCallable)
    void StartThinking();

    UFUNCTION(BlueprintCallable)
    void StopThinking();

    UFUNCTION(BlueprintCallable)
    void Think();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain")
    float ThinkInterval = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugBrain = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain")
    TObjectPtr<UEnemyRoleDataAsset> RoleAsset;

    UPROPERTY(BlueprintReadOnly)
    EEnemyRole EnemyRole;

    UPROPERTY(BlueprintReadOnly)
    FEnemyBrainContext Context;

    UPROPERTY(BlueprintReadOnly)
    FDecisionResult CurrentDecision;

    UPROPERTY(BlueprintAssignable)
    FOnDecisionMade OnDecisionMade;

    UFUNCTION(BlueprintCallable)
    void SetTarget(AActor* NewTarget);

protected:

    void UpdateContext();

    void EvaluateActions();

    float ScoreApproach() const;
    float ScoreRetreat() const;
    float ScoreStrafe() const;
    float ScoreLightAttack() const;
    float ScoreHeavyAttack() const;
    float ScoreDodge() const;
    float ScoreWait() const;

    void ExecuteDecision();

private:
    UPROPERTY()
    UCombatComponent* Combat;

    UPROPERTY()
    UDefenseComponent* Defense;

    UPROPERTY()
    UHealthComponent* Health;

    UPROPERTY()
    UEnemyMovementComponent* Movement;
    FTimerHandle ThinkTimer;
};