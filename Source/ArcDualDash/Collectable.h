#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

#include "Collectable.generated.h"

/**
 * Collectable (ring/coin)
 * notes: Sphere + Box as overlap triggers; Mesh is visual only
 */
UCLASS()
class ARCDUALDASH_API ACollectable : public AActor
{
    GENERATED_BODY()

public:
    ACollectable();

protected:
    virtual void BeginPlay() override;

    // components (show in BP as Inherited)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    USceneComponent* RootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    USphereComponent* Sphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    UBoxComponent* Box; // notes: debug/backup trigger (larger volume, easy to see)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Mesh;

    // overlap handlers
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnActorOverlap(AActor* OverlappedActor, AActor* OtherActor); // notes: extra signal

    void ConsumePickup(); // notes: hide + disable + destroy

    // notes: quick helper to log distance to a pawn (capsule center)
    void DebugLogDistanceToPlayer(APawn* Pawn) const;
};
