#include "Collectable.h"
#include "ArcCharacter.h"
#include "ArcPlayerState.h"
#include "Components/CapsuleComponent.h"

ACollectable::ACollectable()
{
    PrimaryActorTick.bCanEverTick = false;

    // notes: actor-level collision enabled so components can query
    SetActorEnableCollision(true);

    // --- root ---
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(RootComp);

    // --- trigger sphere (overlap with Pawn only) ---
    Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    Sphere->SetupAttachment(RootComp);
    Sphere->InitSphereRadius(100.f); // tune in BP via component details
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);   // notes: hard reset
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Sphere->SetGenerateOverlapEvents(true);

    // --- visual mesh (no collision) ---
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetupAttachment(RootComp);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Tags.Add(FName("Collectable"));
}

void ACollectable::BeginPlay()
{
    Super::BeginPlay();

    // notes: hard guards to avoid crashes when BP/instance lost components
    if (!ensureAlwaysMsgf(IsValid(Sphere), TEXT("[Collectable] Sphere is null on %s"), *GetName()))
    {
        return;
    }
    if (Mesh && Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
    {
        // notes: visual should never block/overlap – enforce once
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    Sphere->SetGenerateOverlapEvents(true);
    Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnSphereBeginOverlap);

    // notes: recompute initial overlaps (safe; sometimes helpful after editor reloads)
    Sphere->UpdateOverlaps();
    UpdateOverlaps(false);
}

void ACollectable::OnSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    // notes: only react to Pawns (players/bots)
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn)
    {
        return;
    }

    // optional: award score to this pawn's PlayerState if present
    if (APlayerState* PS = Pawn->GetPlayerState())
    {
        if (AArcPlayerState* ArcPS = Cast<AArcPlayerState>(PS))
        {
            ArcPS->AddScoreInt(ScoreValue);
        }
        else
        {
            // notes: even if not our custom PS, try to bump base float score for safety
            const float NewScore = PS->GetScore() + static_cast<float>(ScoreValue);
            PS->SetScore(NewScore);
        }
    }

    // notes: if it's our character, apply effects (boost etc.)
    if (AArcCharacter* ArcChar = Cast<AArcCharacter>(Pawn))
    {
        ApplyEffects(ArcChar);
    }

    UE_LOG(LogTemp, Log, TEXT("[Collectable] picked by %s (Score +%d, Boost=%s, Dur=%.1fs)"),
        *GetNameSafe(OtherActor), ScoreValue, bGivesSpeedBoost ? TEXT("Yes") : TEXT("No"), BoostDuration);

    ConsumePickup();
}

void ACollectable::ApplyEffects_Implementation(AArcCharacter* Picker)
{
    if (!Picker) return;

    // notes: o speed boost
    if (bGivesSpeedBoost)
    {
        Picker->StartTimedPowerUp(BoostDuration);
    }

    // notes:  health
    if (HealthAmount != 0)
    {
        Picker->AddHealth(HealthAmount);
    }

    // notes:  ammo
    if (AmmoAmount != 0)
    {
        Picker->AddAmmo(AmmoAmount);
    }
}


void ACollectable::ConsumePickup()
{
    // notes: hide + disable collisions and remove shortly
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    SetLifeSpan(0.1f);
}
