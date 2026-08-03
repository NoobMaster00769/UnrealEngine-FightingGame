#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoleProfile.h"
#include "EnemyRoleDataAsset.generated.h"


UENUM(BlueprintType)
enum class EEnemyRole : uint8
{
	Aggressor,
	Defender,
	Duelist,
	Hunter,
	Coward
};

class UUserDefinedStruct;

UCLASS(BlueprintType)
class FIGHTINGGAME_API UEnemyRoleDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EEnemyRole Role = EEnemyRole::Aggressor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRoleProfile RoleProfile;
};