#include "HitReactionComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

UHitReactionComponent::UHitReactionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHitReactionComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UHitReactionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);
}

void UHitReactionComponent::ReactToHit(EHitDirection Direction)
{
    if (bIsReacting)
        return;

    bIsReacting = true;
    CurrentDirection = Direction;

    OnHitReactionStarted.Broadcast();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ReactionTimer,
            this,
            &UHitReactionComponent::FinishReaction,
            HitReactionTime,
            false
        );
    }
}


void UHitReactionComponent::FinishReaction()
{
    bIsReacting = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReactionTimer);
    }

    OnHitReactionFinished.Broadcast();
}

bool UHitReactionComponent::IsReacting() const
{
    return bIsReacting;
}

EHitDirection UHitReactionComponent::GetHitDirection() const
{
    return CurrentDirection;
}
void UHitReactionComponent::CancelReaction()
{
    bIsReacting = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReactionTimer);
    }
}