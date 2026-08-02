#include "CombatPerceptionComponent.h"
#include "CombatComponent.h"
#include "DefenseComponent.h"
#include "HitReactionComponent.h"
#include "GameFramework/Actor.h"

UCombatPerceptionComponent::UCombatPerceptionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = PerceptionUpdateInterval;
}

void UCombatPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickInterval(PerceptionUpdateInterval);
}

void UCombatPerceptionComponent::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void UCombatPerceptionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateSnapshot();
}

void UCombatPerceptionComponent::UpdateSnapshot()
{
	if (!TargetActor)
	{
		Snapshot.bTargetValid = false;
		return;
	}

	Snapshot.bTargetValid = true;

	UCombatComponent* TargetCombat = TargetActor->FindComponentByClass<UCombatComponent>();
	UDefenseComponent* TargetDefense = TargetActor->FindComponentByClass<UDefenseComponent>();
	UHitReactionComponent* TargetHitReaction = TargetActor->FindComponentByClass<UHitReactionComponent>();

	if (TargetCombat)
	{
		Snapshot.bIsAttacking = TargetCombat->IsAttacking();
		Snapshot.bWeaponCollisionActive = TargetCombat->IsWeaponCollisionActive();
		Snapshot.bCanAttack = TargetCombat->CanAttack();
		Snapshot.bIsRecovering = Snapshot.bIsAttacking && !Snapshot.bWeaponCollisionActive;
	}

	if (TargetDefense)
	{
		Snapshot.bIsDodging = TargetDefense->IsDodging();
		Snapshot.bCanDodge = TargetDefense->CanDodge();
		Snapshot.bIsInvulnerable = TargetDefense->IsInvulnerable();
	}

	if (TargetHitReaction)
	{
		Snapshot.bIsReacting = TargetHitReaction->IsReacting();
		Snapshot.HitDirection = TargetHitReaction->GetHitDirection();
	}

	if (AActor* Owner = GetOwner())
	{
		Snapshot.bIsFacingTarget =
			IsActorFacingActor(Owner, TargetActor, FacingToleranceDegrees);

		Snapshot.bTargetIsFacingMe =
			IsActorFacingActor(TargetActor, Owner, FacingToleranceDegrees);
	}

	if (bDebugPerception)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Perception] Attacking=%d Recovering=%d CollisionActive=%d Dodging=%d Reacting=%d FacingTarget=%d"),
			Snapshot.bIsAttacking, Snapshot.bIsRecovering, Snapshot.bWeaponCollisionActive,
			Snapshot.bIsDodging, Snapshot.bIsReacting, Snapshot.bIsFacingTarget);
	}
}

bool UCombatPerceptionComponent::IsActorFacingActor(
	const AActor* Observer,
	const AActor* Target,
	float ToleranceDegrees) const
{
	if (!Observer || !Target)
	{
		return false;
	}

	const FVector ToTarget =
		(Target->GetActorLocation() - Observer->GetActorLocation()).GetSafeNormal();

	const FVector Forward = Observer->GetActorForwardVector();

	const float DotProduct = FVector::DotProduct(Forward, ToTarget);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

	return AngleDegrees <= ToleranceDegrees;
}