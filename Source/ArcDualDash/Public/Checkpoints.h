#pragma once

// ============================================================================
// Checkpoints.h
// purpose: Box trigger checkpoint for lap system.
// used by: MyCar (AMyCar::LapCheckpoint).  KM
// ============================================================================
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Checkpoints.generated.h"

UCLASS()
class ARCDUALDASH_API ACheckpoints : public AActor
{
    GENERATED_BODY()

public:
    // notes: ctor sets up Box trigger as overlap volume
    ACheckpoints();

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

public:
    virtual void Tick(float DeltaTime) override;

    // notes: overlap handlers per lab
    UFUNCTION()
    void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // --- components / params ---
    UPROPERTY(EditAnywhere, Category = "Checkpoint")
    UBoxComponent* Volume;

    // notes: checkpoint ordinal in this track (1..MaxCheckPoints)
    UPROPERTY(EditAnywhere, Category = "Checkpoint")
    int32 CheckPointNo = 1;

    // notes: total checkpoints used on this track
    UPROPERTY(EditAnywhere, Category = "Checkpoint")
    int32 MaxCheckPoints = 1;

    // notes: true only on Start/Finish gate
    UPROPERTY(EditAnywhere, Category = "Checkpoint")
    bool bStartFinishLine = false;
};
