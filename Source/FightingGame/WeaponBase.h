#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UBoxComponent;
class UCombatComponent;

UCLASS()
class FIGHTINGGAME_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/*========================================
					COMPONENTS
	========================================*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UBoxComponent* WeaponCollision;

	/*========================================
					COLLISION API
	========================================*/
	UFUNCTION(BlueprintCallable, Category = "Weapon|Collision")
	void EnableCollision();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Collision")
	void DisableCollision();

	UFUNCTION(BlueprintPure, Category = "Weapon|Collision")
	UBoxComponent* GetCollisionBox() const;

	/*========================================
					OWNER LINK
	========================================*/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetOwningCombatComponent(UCombatComponent* InCombat);

private:

	void PerformSweep();

	UPROPERTY()
	UCombatComponent* OwningCombatComponent = nullptr;

	bool bIsSweeping = false;

	FTransform PreviousSweepTransform;
};