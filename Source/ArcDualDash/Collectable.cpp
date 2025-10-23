// ============================================================================
// Collectable.cpp
// notes: Overlap-driven pickup. I award score (if PS exists), apply effects to
//        ArcCharacter, then hide and destroy myself. KM
// ============================================================================

#include "Collectable.h"
#include "ArcCharacter.h"      // notes: For applying boost/health/ammo. KM
#include "ArcPlayerState.h"    // notes: For AddScoreInt when available. KM
#include "Components/CapsuleComponent.h" // notes: Used by character; not required here but kept for context. KM

// ============================================================================
// Ctor: components + default collision for a trigger pickup
// ============================================================================
ACollectable::ACollectable()
{
    PrimaryActorTick.bCanEverTick = false;

    SetActorEnableCollision(true); // notes: Let components receive overlaps. KM

    // Root
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(RootComp);

    // Trigger sphere: overlap only with Pawns
    Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    Sphere->SetupAttachment(RootComp);
    Sphere->InitSphereRadius(100.f); // notes: Tune in BP. KM
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore); // notes: Reset first. KM
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Sphere->SetGenerateOverlapEvents(true);

    // Visual mesh: no collision
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetupAttachment(RootComp);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Tags.Add(FName("Collectable")); // notes: Helpful for debugging/filters. KM
}

// ============================================================================
// BeginPlay: bind overlap and enforce no-collision on mesh
// ============================================================================
void ACollectable::BeginPlay()
{
    Super::BeginPlay();

    // Guard against broken BP instances. KM
    if (!ensureAlwaysMsgf(IsValid(Sphere), TEXT("[Collectable] Sphere is null on %s"), *GetName()))
    {
        return;
    }

    // Visual should never block or overlap. KM
    if (Mesh && Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    Sphere->SetGenerateOverlapEvents(true);
    Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnSphereBeginOverlap);

    // Recompute overlaps (helps after editor reload). KM
    Sphere->UpdateOverlaps();
    UpdateOverlaps(false);
}

// ============================================================================
// Overlap: give score (if any) and apply effects to our character
// ============================================================================
void ACollectable::OnSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    // Only Pawns can pick this. KM
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn)
    {
        return;
    }

    // Award score to the picker’s PlayerState (prefer our ArcPlayerState).
    if (APlayerState* PS = Pawn->GetPlayerState())
    {
        if (AArcPlayerState* ArcPS = Cast<AArcPlayerState>(PS))
        {
            ArcPS->AddScoreInt(ScoreValue); // notes: Keeps score path unified. KM
        }
        else
        {
            // Fallback: bump base float score so non-custom PS still gains points. KM
            const float NewScore = PS->GetScore() + static_cast<float>(ScoreValue);
            PS->SetScore(NewScore);
        }
    }

    // Apply effects if the picker is our character.
    if (AArcCharacter* ArcChar = Cast<AArcCharacter>(Pawn))
    {
        ApplyEffects(ArcChar);
    }

    UE_LOG(LogTemp, Log, TEXT("[Collectable] picked by %s (Score +%d, Boost=%s, Dur=%.1fs)"),
        *GetNameSafe(OtherActor), ScoreValue, bGivesSpeedBoost ? TEXT("Yes") : TEXT("No"), BoostDuration);

    ConsumePickup(); // notes: Remove the pickup. KM
}

// ============================================================================
// ApplyEffects: boost/health/ammo; easy to extend in BP
// ============================================================================
void ACollectable::ApplyEffects_Implementation(AArcCharacter* Picker)
{
    if (!Picker) return;

    if (bGivesSpeedBoost)
    {
        Picker->StartTimedPowerUp(BoostDuration); // notes: Temporary speed change. KM
    }

    if (HealthAmount != 0)
    {
        Picker->AddHealth(HealthAmount); // notes: Clamped in character. KM
    }

    if (AmmoAmount != 0)
    {
        Picker->AddAmmo(AmmoAmount); // notes: Clamped in character. KM
    }
}

// ============================================================================
// Consume: hide, disable, and destroy soon
// ============================================================================
void ACollectable::ConsumePickup()
{
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    SetLifeSpan(0.1f); // notes: Short lifetime so it disappears fast. KM
}
