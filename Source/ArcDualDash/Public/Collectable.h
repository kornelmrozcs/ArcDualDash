#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Collectable.generated.h"

class AMyCar;

UCLASS()
class ARCDUALDASH_API ACollectable : public AActor
{
	GENERATED_BODY()

public:
	ACollectable();

protected:
	virtual void BeginPlay() override;

	// --- components (show as Inherited in BP) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collectable", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh = nullptr;

public:
	// --- gameplay params (edit per BP instance) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collectable|Score")
	int32 ScoreValue = 1; // e.g., ring worth 1pt

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collectable|Boost")
	bool bGivesSpeedBoost = false; // true for speed ring

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collectable|Boost", meta = (EditCondition = "bGivesSpeedBoost", ClampMin = "0.1", ClampMax = "30.0"))
	float BoostDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collectable|Boost", meta = (EditCondition = "bGivesSpeedBoost", ClampMin = "1.0", ClampMax = "5.0"))
	float BoostScale = 1.75f;

protected:
	// --- overlap ---
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ConsumePickup(); // notes: hide + disable + quick destroy
};
