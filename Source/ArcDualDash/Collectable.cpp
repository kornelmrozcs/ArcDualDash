#include "Collectable.h"
#include "ArcCharacter.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ACollectable::ACollectable()
{
    PrimaryActorTick.bCanEverTick = false;

    // safety: make sure actor can collide at all
    SetActorEnableCollision(true); // notes: actor-level flag

    // --- root ---
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(RootComp);

    // sphere trigger
    Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    Sphere->SetupAttachment(RootComp);
    Sphere->InitSphereRadius(120.f);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);         // ok
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);     // hard reset
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Sphere->SetGenerateOverlapEvents(true);
    /*
    // box trigger (backup)
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    Box->SetupAttachment(RootComp);
    Box->SetBoxExtent(FVector(120.f));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldDynamic);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Box->SetGenerateOverlapEvents(true);
    */

    // --- visual mesh ---
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetupAttachment(RootComp);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Tags.Add(FName("Collectable"));
}

void ACollectable::BeginPlay()
{
    Super::BeginPlay();

    // bind overlaps
    Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnSphereBeginOverlap);
    Box->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnBoxBeginOverlap);
    OnActorBeginOverlap.AddDynamic(this, &ACollectable::OnActorOverlap);

    // debug dump of settings
    UE_LOG(LogTemp, Log, TEXT("[Collectable] BeginPlay: Sphere R=%.1f, CE=%d, Obj=%d, PawnResp=%d, Events=%d"),
        Sphere->GetUnscaledSphereRadius(),
        (int32)Sphere->GetCollisionEnabled(),
        (int32)Sphere->GetCollisionObjectType(),
        (int32)Sphere->GetCollisionResponseToChannel(ECC_Pawn),
        Sphere->GetGenerateOverlapEvents());
    /*
    // draw debug volume so I can SEE the trigger in game
    DrawDebugSphere(GetWorld(), Sphere->GetComponentLocation(), Sphere->GetUnscaledSphereRadius(), 24, FColor::Purple, false, 9999.f, 0, 2.f);
    DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetUnscaledBoxExtent(), FColor::Cyan, false, 9999.f, 0, 2.f);
    */
    // force overlap evaluation now
    Sphere->UpdateOverlaps();
    Box->UpdateOverlaps();
    UpdateOverlaps(false);

    /*// TEMP: snap first collectable onto Player0 to guarantee intersection
    static bool bDidSnapOnce = false;
    if (!bDidSnapOnce)
    {
        if (APlayerController* PC0 = GetWorld()->GetFirstPlayerController())
        {
            if (APawn* P0 = PC0->GetPawn())
            {
                FVector P = P0->GetActorLocation();
                SetActorLocation(P + FVector(0, 0, -20.0f)); // notes: cut the capsule for sure
                UE_LOG(LogTemp, Warning, TEXT("[Collectable] TEMP snap to Player0 @ %s"), *GetActorLocation().ToCompactString());
                DebugLogDistanceToPlayer(P0);
                Sphere->UpdateOverlaps();
                Box->UpdateOverlaps();
                UpdateOverlaps(false);
            }
        }
        bDidSnapOnce = true;
    }*/
}

void ACollectable::OnSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    if (APawn* Pawn = Cast<APawn>(OtherActor))
    {
        if (AArcCharacter* ArcChar = Cast<AArcCharacter>(Pawn))
        {
            // notes: trigger 5s boost; tune in BP via BoostedMaxWalkSpeed
            ArcChar->StartTimedPowerUp(5.f);
        }

        UE_LOG(LogTemp, Log, TEXT("[Collectable] picked by %s"), *OtherActor->GetName());
        ConsumePickup();
    }


}

void ACollectable::OnBoxBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    if (APawn* Pawn = Cast<APawn>(OtherActor))
    {
        if (AArcCharacter* ArcChar = Cast<AArcCharacter>(Pawn))
        {
            // notes: trigger 5s boost; tune in BP via BoostedMaxWalkSpeed
            ArcChar->StartTimedPowerUp(5.f);
        }

        UE_LOG(LogTemp, Log, TEXT("[Collectable] picked by %s"), *OtherActor->GetName());
        ConsumePickup();
    }


}

void ACollectable::OnActorOverlap(AActor* /*OverlappedActor*/, AActor* OtherActor)
{
    UE_LOG(LogTemp, Verbose, TEXT("[Collectable] ACTOR overlap with %s"), *GetNameSafe(OtherActor));
}

void ACollectable::ConsumePickup()
{
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    SetLifeSpan(0.1f);
}

void ACollectable::DebugLogDistanceToPlayer(APawn* Pawn) const
{
    if (!Pawn) return;
    const FVector A = Pawn->GetActorLocation();
    const FVector B = GetActorLocation();
    const float Dist = FVector::Dist(A, B);
    UE_LOG(LogTemp, Log, TEXT("[Collectable] Dist to Player0: %.1f (A=%s, B=%s)"),
        Dist, *A.ToCompactString(), *B.ToCompactString());
}
