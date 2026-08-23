#include "WeaponBase.h"
#include "Components/BoxComponent.h"
#include "CombatComponent.h"
#include "CollisionShape.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
	WeaponCollision->SetupAttachment(Root);

	// Sweeps are manual world queries; the component itself never
	// needs to physically collide or generate overlap events anymore.
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponCollision->SetGenerateOverlapEvents(false);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	DisableCollision();
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsSweeping)
		return;

	PerformSweep();

	PreviousSweepTransform = WeaponCollision->GetComponentTransform();
}

void AWeaponBase::EnableCollision()
{
	if (!WeaponCollision)
		return;

	PreviousSweepTransform = WeaponCollision->GetComponentTransform();
	bIsSweeping = true;
}

void AWeaponBase::DisableCollision()
{
	bIsSweeping = false;
}

UBoxComponent* AWeaponBase::GetCollisionBox() const
{
	return WeaponCollision;
}

void AWeaponBase::SetOwningCombatComponent(UCombatComponent* InCombat)
{
	OwningCombatComponent = InCombat;
}

void AWeaponBase::PerformSweep()
{
    if (!OwningCombatComponent)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[TEST] Weapon Sweep ABORTED: No OwningCombatComponent")
        );

        return;
    }


    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[TEST] Weapon PerformSweep")
    );

    const FTransform CurrentTransform =
        WeaponCollision->GetComponentTransform();

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(WeaponSweep),
        false
    );

    QueryParams.AddIgnoredActor(this);

    if (GetOwner())
    {
        QueryParams.AddIgnoredActor(GetOwner());
    }

    TArray<FHitResult> Hits;

    const bool bSweepExecuted =
        GetWorld()->SweepMultiByChannel(
            Hits,
            PreviousSweepTransform.GetLocation(),
            CurrentTransform.GetLocation(),
            CurrentTransform.GetRotation(),
            ECC_Pawn,
            FCollisionShape::MakeBox(
                WeaponCollision->GetScaledBoxExtent()
            ),
            QueryParams
        );


    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[TEST] Sweep executed=%s | Hits=%d | Previous=%s | Current=%s"
        ),
        bSweepExecuted ? TEXT("TRUE") : TEXT("FALSE"),
        Hits.Num(),
        *PreviousSweepTransform.GetLocation().ToString(),
        *CurrentTransform.GetLocation().ToString()
    );

    // ---------------------------------------------------------
    // Process every detected hit
    // ---------------------------------------------------------

    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        UPrimitiveComponent* HitComp = Hit.GetComponent();

        if (!HitActor || !HitComp)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[TEST] Sweep returned invalid HitActor or HitComponent")
            );

            continue;
        }


        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[TEST] WEAPON HIT DETECTED: %s | Component: %s | Bone: %s"
            ),
            *HitActor->GetName(),
            *HitComp->GetName(),
            *Hit.BoneName.ToString()
        );

        OwningCombatComponent->RegisterHit(
            HitActor,
            HitComp,
            Hit.ImpactPoint,
            Hit.ImpactNormal,
            GetActorForwardVector(),
            Hit.BoneName
        );
    }


    PreviousSweepTransform =
        WeaponCollision->GetComponentTransform();
}