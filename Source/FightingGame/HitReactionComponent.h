#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitReactionComponent.generated.h"

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
    Front,
    Back,
    Left,
    Right
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitReactionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitReactionFinished);

UCLASS(ClassGroup = (Combat), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UHitReactionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UHitReactionComponent();

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /*=========================================
                HIT REACTION
    =========================================*/

    UFUNCTION(BlueprintCallable)
    void ReactToHit(EHitDirection Direction);

    UFUNCTION(BlueprintCallable)
    void FinishReaction();

    UFUNCTION(BlueprintCallable)
    void CancelReaction();

    UFUNCTION(BlueprintPure)
    bool IsReacting() const;

    UFUNCTION(BlueprintPure)
    EHitDirection GetHitDirection() const;

    UPROPERTY(VisibleAnywhere, Category = "Hit")
    EHitDirection CurrentDirection = EHitDirection::Front;

public:

    UPROPERTY(BlueprintAssignable)
    FOnHitReactionStarted OnHitReactionStarted;

    UPROPERTY(BlueprintAssignable)
    FOnHitReactionFinished OnHitReactionFinished;

private:

    UPROPERTY(VisibleAnywhere, Category = "Hit")
    bool bIsReacting = false;

    UPROPERTY(EditAnywhere, Category = "Hit")
    float HitReactionTime = 0.35f;

    FTimerHandle ReactionTimer;
};