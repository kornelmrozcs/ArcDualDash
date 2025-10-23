#pragma once

// ==========================================================================
// Collectable.h
// notes: Base pickup (ring/coin). Sphere overlaps Pawns; Mesh is visual only.
//        I keep simple params so we can build BP variants (score, boost, health, ammo).
//        Used by: ArcCharacter (effects), ArcPlayerState (score). KM
// ==========================================================================

#include "CoreMinimal.h"                  // notes: Core UE types. KM
#include "GameFramework/Actor.h"          // notes: Pickup is an Actor. KM
#include "Components/SphereComponent.h"   // notes: Trigger volume. KM
#include "Components/StaticMeshComponent.h" // notes: Visual only. KM

#include "Collectable.generated.h"

class AArcCharacter; // notes: I apply effects to our character. KM

/**
 * notes: Extensible pickup. I expose params and a BP-native hook ApplyEffects()
 *        so we can add behavior per BP class. KM
 */
UCLASS()
class ARCDUALDASH_API ACollectable : public AActor
{
    GENERATED_BODY()

public:
    ACollectable(); // notes: Build components and set overlap defaults. KM

protected:
    virtual void BeginPlay() override; // notes: Bind overlap and enforce no-collision on mesh. KM

    // =========================================================================
    // Components (show as Inherited in BP so we don’t duplicate them)
    // =========================================================================
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    USceneComponent* RootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    USphereComponent* Sphere; // notes: Only overlaps Pawns. KM

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Mesh; // notes: Visual only; no collision. KM

public:
    // =========================================================================
    // Gameplay params (tuned per BP instance)
    // =========================================================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Score")
    int32 ScoreValue = 1; // notes: How many points I add. Used by: ArcPlayerState. KM

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost")
    bool bGivesSpeedBoost = false; // notes: If true, I trigger speed boost on the picker. KM

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost", meta = (EditCondition = "bGivesSpeedBoost", ClampMin = "0.1", ClampMax = "30.0"))
    float BoostDuration = 5.f; // notes: Boost seconds passed to ArcCharacter. KM

    // notes: Main hook to stack more effects in BP later (damage, etc.). Called on overlap. KM
    UFUNCTION(BlueprintNativeEvent, Category = "Collectable")
    void ApplyEffects(AArcCharacter* Picker);
    virtual void ApplyEffects_Implementation(AArcCharacter* Picker);

    // notes: Simple stat effects so I can build health/ammo pickups without new C++. KM
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Health")
    int32 HealthAmount = 0;   // e.g., +25. Used by: ArcCharacter::AddHealth. KM

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Ammo")
    int32 AmmoAmount = 0;     // e.g., +15. Used by: ArcCharacter::AddAmmo. KM

protected:
    // =========================================================================
    // Overlap
    // =========================================================================
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult); // notes: Entry point; awards score and applies effects. KM

    void ConsumePickup(); // notes: Hide, disable, quick destroy so it disappears cleanly. KM
};
