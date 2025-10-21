#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Collectable.generated.h"

class AArcCharacter;

/**
 * Collectable (base ring/coin)
 * notes: Sphere fires overlaps with Pawn; Mesh is visual only.
 *        Designed to be extensible: params + ApplyEffects() hook (BlueprintNativeEvent).
 */
UCLASS()
class ARCDUALDASH_API ACollectable : public AActor
{
    GENERATED_BODY()

public:
    ACollectable();

protected:
    virtual void BeginPlay() override;

    // --- components (show in BP as Inherited) ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    USceneComponent* RootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    USphereComponent* Sphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Mesh;

public:
    // --- gameplay params (configure per-BP instance) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Score")
    int32 ScoreValue = 1; // BP_RingScore=1, BP_RingBoost=5

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost")
    bool bGivesSpeedBoost = false; // BP_RingBoost=true, BP_RingScore=false

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost", meta = (EditCondition = "bGivesSpeedBoost", ClampMin = "0.1", ClampMax = "30.0"))
    float BoostDuration = 5.f; // seconds

    // notes: main hook for future effects (damage, health, ammo, etc.)
    UFUNCTION(BlueprintNativeEvent, Category = "Collectable")
    void ApplyEffects(AArcCharacter* Picker);
    virtual void ApplyEffects_Implementation(AArcCharacter* Picker);

    // notes: health/ammo effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Health")
    int32 HealthAmount = 0;   // e.g. +25 for health pack

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Ammo")
    int32 AmmoAmount = 0;     // e.g. +15 for ammo box


protected:
    // --- overlap handler ---
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    void ConsumePickup(); // notes: hide + disable + quick destroy
};
