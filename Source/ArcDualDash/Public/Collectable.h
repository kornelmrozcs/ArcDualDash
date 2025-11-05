#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Collectable.generated.h"

class AMyCar;

/**
 * Collectable (ring / punkt / boost)
 * notes: Sphere overlaps Vehicle (albo preset "Pickup"). Mesh — tylko wizual.
 */
UCLASS()
class ARCDUALDASH_API ACollectable : public AActor
{
	GENERATED_BODY()

public:
	ACollectable();

protected:
	virtual void BeginPlay() override;

	// --- Components (visible as Inherited) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:
	// --- Gameplay params (tweak in BP) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Score")
	int32 ScoreValue = 1;          // np. zwyk³y ring = 1

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost")
	bool bGivesSpeedBoost = true;  // ring-boost = true

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost", meta = (EditCondition = "bGivesSpeedBoost", ClampMin = "0.1", ClampMax = "10.0"))
	float BoostDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable|Boost", meta = (EditCondition = "bGivesSpeedBoost", ClampMin = "1000", ClampMax = "10000"))
	float BoostForce = 1000.f;

protected:
	// --- overlap handler ---
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	void ConsumePickup(); // notes: hide + disable + quick destroy
};
