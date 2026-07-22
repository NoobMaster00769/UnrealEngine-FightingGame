#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* WeaponCollision;

	/*========================================
			   COLLISION API
	========================================*/

	UFUNCTION(BlueprintCallable)
	void EnableCollision();

	UFUNCTION(BlueprintCallable)
	void DisableCollision();

	UFUNCTION(BlueprintPure)
	UBoxComponent* GetCollisionBox() const;
};