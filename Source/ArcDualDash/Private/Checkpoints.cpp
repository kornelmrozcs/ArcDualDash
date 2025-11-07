// ============================================================================
// Checkpoints.cpp
// notes: Box trigger that calls AMyCar::LapCheckpoint on overlap. Mirrors lab.
// ============================================================================
#include "Checkpoints.h"
#include "MyCar.h" // notes: to call LapCheckpoint on overlap

// Sets default values
ACheckpoints::ACheckpoints()
{
    PrimaryActorTick.bCanEverTick = true;

    // notes: create BOX TRIGGER Volume per lab
    Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
    Volume->InitBoxExtent(FVector(100.f, 400.f, 500.f)); // tune in editor
    Volume->SetCollisionResponseToAllChannels(ECR_Overlap);
    Volume->SetGenerateOverlapEvents(true);
    SetRootComponent(Volume);
}

void ACheckpoints::BeginPlay()
{
    Super::BeginPlay();
}

void ACheckpoints::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // notes: bind overlap delegates
    Volume->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoints::OnVolumeBeginOverlap);
    Volume->OnComponentEndOverlap.AddDynamic(this, &ACheckpoints::OnVolumeEndOverlap);
}

void ACheckpoints::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACheckpoints::OnVolumeBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
    if (AMyCar* Car = Cast<AMyCar>(OtherActor))
    {
        Car->LapCheckpoint(CheckPointNo, MaxCheckPoints, bStartFinishLine);
        Car->SetLastCheckpoint(this); // <— NEW line
    }
}

void ACheckpoints::OnVolumeEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* /*OtherActor*/,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
    // notes: Triggered when vehicle exits Box trigger (not used in lab)
}
