// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "InputActionValue.h"
#include "ChaosVehicleMovementComponent.h"
#include "MyCar.generated.h"

UCLASS()
class ARCDUALDASH_API AMyCar : public AWheeledVehiclePawn
{
	GENERATED_BODY()
public:
	AMyCar();                            // notes: enable Tick (boost needs per-frame force)

	void BeginPlay();
	virtual void Tick(float DeltaSeconds) override;   // notes: apply boost force every frame
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- Input (P1) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	/** Move Foward Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	/** Brake Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* HandbrakeAction;

	void Move(const FInputActionValue& Value);
	void MoveEnd();
	void OnHandbrakePressed();
	void OnHandbrakeReleased();

	// --- Race laps (lab logic) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 Lap = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 CurrentCheckpoint = 0;

	UFUNCTION(BlueprintCallable, Category = "Race|Laps")
	void LapCheckpoint(int32 _CheckpointNo, int32 _MaxCheckpoint, bool _bStartFinishLine);

	// --- Keyboard proxy for P2 (Arrows + Right Ctrl) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|P2")
	class UInputMappingContext* ProxyMappingContext_P2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|P2")
	class UInputAction* MoveAction_P2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|P2")
	class UInputAction* HandbrakeAction_P2 = nullptr;

	void Move_P2(const FInputActionValue& Value);
	void MoveEnd_P2();
	void OnHandbrakePressed_P2();
	void OnHandbrakeReleased_P2();

	// --- PowerUps / Score (u¿ywane przez Collectable) ---
	UFUNCTION(BlueprintCallable, Category = "PowerUp")
	void StartSpeedBoost(float DurationSeconds = 2.5f, float Force = 1800000.f); // notes: start nitro

	UFUNCTION(BlueprintCallable, Category = "PowerUp")
	void EndSpeedBoost(); // notes: stop nitro

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 AddScore(int32 Delta); // notes: simple per-car points

	// Tuning
	UPROPERTY(EditAnywhere, Category = "PowerUp")
	float BoostForce = 1000.f;    // 

	UPROPERTY(EditAnywhere, Category = "PowerUp")
	float BoostDragScale = 0.6f;     // 

	UPROPERTY(EditAnywhere, Category = "PowerUp")
	float BoostDurationDefault = 2.5f; // 

private:
	// Boost internals
	bool bBoostActive = false;
	float SavedDragCoefficient = 0.f;
	FTimerHandle BoostTimer;

	// Simple local score (mo¿na przenieœæ do PlayerState póŸniej)
	int32 Score = 0;
};
