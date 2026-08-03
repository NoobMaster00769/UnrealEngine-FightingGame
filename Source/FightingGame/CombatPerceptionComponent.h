#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitReactionComponent.h"
#include "CombatPerceptionComponent.generated.h"

class UCombatComponent;
class UDefenseComponent;
class UHitReactionComponent;

USTRUCT(BlueprintType)
struct FCombatPerceptionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bIsAttacking = false;
	UPROPERTY(BlueprintReadOnly) bool bWeaponCollisionActive = false;
	UPROPERTY(BlueprintReadOnly) bool bIsRecovering = false;      // derived: attacking && !collision active
	UPROPERTY(BlueprintReadOnly) bool bCanAttack = true;

	UPROPERTY(BlueprintReadOnly) bool bIsDodging = false;
	UPROPERTY(BlueprintReadOnly) bool bCanDodge = true;
	UPROPERTY(BlueprintReadOnly) bool bIsInvulnerable = false;

	UPROPERTY(BlueprintReadOnly) bool bIsReacting = false;
	UPROPERTY(BlueprintReadOnly) EHitDirection HitDirection = EHitDirection::Front;

	UPROPERTY(BlueprintReadOnly) bool bIsFacingTarget = false;
	UPROPERTY(BlueprintReadOnly) bool bTargetIsFacingMe = false;

	UPROPERTY(BlueprintReadOnly) bool bTargetValid = false;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UCombatPerceptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatPerceptionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure)
	const FCombatPerceptionSnapshot& GetSnapshot() const { return Snapshot; }

	UPROPERTY(EditAnywhere, Category = "Perception")
	float PerceptionUpdateInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugPerception = false;

private:
	void UpdateSnapshot();
	bool IsActorFacingActor(const AActor* Observer, const AActor* Target, float ToleranceDegrees) const;

private:
	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	FCombatPerceptionSnapshot Snapshot;

	UPROPERTY(EditAnywhere, Category = "Perception")
	float FacingToleranceDegrees = 45.f;

public:
	UFUNCTION(BlueprintCallable)
	bool ConsumeDangerLatch();

private:
	bool bDangerLatched = false;
};