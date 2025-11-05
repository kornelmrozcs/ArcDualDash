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

	// --- Input (P1) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	/** Move Foward Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	/** Brake Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* HandbrakeAction;

	// --- Handlers (P1) ---
	void Move(const FInputActionValue& Value);
	void MoveEnd();
	void OnHandbrakePressed();
	void OnHandbrakeReleased();

	// --- Laps (lab) ---
	// notes: current lap number; lab starts at 1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 Lap = 1;

	// notes: last valid checkpoint reached (1..MaxCheckPoints), 0 = none yet
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 CurrentCheckpoint = 0;

	// notes: called by checkpoint volumes when overlapped; enforces order
	UFUNCTION(BlueprintCallable, Category = "Race|Laps")
	void LapCheckpoint(int32 _CheckpointNo, int32 _MaxCheckpoint, bool _bStartFinishLine);

	// --- P2 keyboard proxy (arrows + Right Ctrl) ---
	// notes: I add/bind these only on ControllerId==0 (first local player).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|P2")
	class UInputMappingContext* ProxyMappingContext_P2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|P2")
	class UInputAction* MoveAction_P2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|P2")
	class UInputAction* HandbrakeAction_P2 = nullptr;
	
	// notes: P2 keyboard proxy handlers. I use them only on ControllerId==0 to drive Player 2's car.
	void Move_P2(const FInputActionValue& Value);
	void MoveEnd_P2();
	void OnHandbrakePressed_P2();
	void OnHandbrakeReleased_P2();

	// ===========================
	// Power-up: Speed Boost (used by collectables)
	// ===========================
	// notes: I scale the throttle in Move() while boost is active; simple and stable with Chaos.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp|Speed")
	float DefaultThrottleScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp|Speed")
	float BoostThrottleScale = 1.75f; // tweak feel

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp|Speed")
	float DefaultBoostDuration = 3.0f; // seconds

	// notes: start a temporary speed boost; pickups can override duration/scale
	UFUNCTION(BlueprintCallable, Category = "PowerUp|Speed")
	void StartSpeedBoost(float DurationSeconds = 3.0f, float Scale = 1.75f);

	// notes: stop boost and restore defaults
	UFUNCTION(BlueprintCallable, Category = "PowerUp|Speed")
	void EndSpeedBoost();

	// --- Score (for collectables) ---
	// notes: simple local score; UI can read with GetScoreInt()
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetScoreInt() const { return Score; }

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Delta);

private:
	// notes: used by Move(); I set this in Start/EndSpeedBoost only
	float CurrentThrottleScale = 1.0f;

	FTimerHandle BoostTimer;

	// notes: stored as int for simplicity (racing miniproject)
	int32 Score = 0;

};
