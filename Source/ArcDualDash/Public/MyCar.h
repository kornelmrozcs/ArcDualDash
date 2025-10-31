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
	void BeginPlay();
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
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

	// notes: current lap number; lab starts at 1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 Lap = 1;

	// notes: last valid checkpoint reached (1..MaxCheckPoints), 0 = none yet
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 CurrentCheckpoint = 0;

	// notes: called by checkpoint volumes when overlapped; enforces order
	UFUNCTION(BlueprintCallable, Category = "Race|Laps")
	void LapCheckpoint(int32 _CheckpointNo, int32 _MaxCheckpoint, bool _bStartFinishLine);

};
