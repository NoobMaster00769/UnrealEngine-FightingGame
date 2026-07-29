#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyMovementComponent.generated.h"

class AActor;
class ACharacter;

UENUM(BlueprintType)
enum class EEnemyMovementMode : uint8
{
    None UMETA(DisplayName = "None"),
    Approach UMETA(DisplayName = "Approach"),
    Retreat UMETA(DisplayName = "Retreat"),
    Strafe UMETA(DisplayName = "Strafe")
};

UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class FIGHTINGGAME_API UEnemyMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UEnemyMovementComponent();

protected:

    virtual void BeginPlay() override;

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

public:

    UFUNCTION(BlueprintCallable)
    void ApproachTarget(AActor* Target);

    UFUNCTION(BlueprintCallable)
    void RetreatFromTarget(AActor* Target);

    UFUNCTION(BlueprintCallable)
    void StrafeAroundTarget(AActor* Target);

    UFUNCTION(BlueprintCallable)
    void StopMovement();

private:

    UPROPERTY()
    ACharacter* OwnerCharacter = nullptr;

    UPROPERTY()
    AActor* CurrentTarget = nullptr;

    EEnemyMovementMode CurrentMovementMode = EEnemyMovementMode::None;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float MovementSpeed = 1.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float RotationInterpSpeed = 8.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    bool bStrafeLeft = true;

    UPROPERTY(EditAnywhere)
    bool bDebugMovement = true;
};