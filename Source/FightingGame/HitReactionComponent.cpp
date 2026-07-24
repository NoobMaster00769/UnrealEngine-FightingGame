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
}


void UHitReactionComponent::FinishReaction()
{
    bIsReacting = false;

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