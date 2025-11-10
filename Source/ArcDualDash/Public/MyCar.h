#pragma once

#include "Checkpoints.h"
#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "InputActionValue.h"
#include "ChaosVehicleMovementComponent.h"

class UBoxComponent;

#include "MyCar.generated.h"

// ---------------------------------------------------------
// Local per-player HUD events
// ---------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLapChangedLocal, int32, NewLap, int32, TotalLaps);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdatedLocal, float, NewTime);

UCLASS()
class ARCDUALDASH_API AMyCar : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	AMyCar();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- Player Identity ---
	UPROPERTY(BlueprintReadOnly, Category = "Race|Player")
	int32 PlayerID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Race|Player")
	bool bIsAI = false;

	// --- Input (P1) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* HandbrakeAction;

	void Move(const FInputActionValue& Value);
	void MoveEnd();
	void OnHandbrakePressed();
	void OnHandbrakeReleased();

	// --- Race laps / checkpoints ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 Lap = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Laps")
	int32 CurrentCheckpointIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Race|Laps")
	void LapCheckpoint(int32 CheckpointNo, int32 MaxCheckpoint, bool bStartFinishLine);

	// --- Leaderboard tracking ---
	UPROPERTY(BlueprintReadOnly, Category = "Race|Progress")
	float DistanceToNextCheckpoint = 0.f;

	UPROPERTY()
	TArray<ACheckpoints*> AllCheckpoints;

	// --- Keyboard proxy for P2 ---
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

	// --- PowerUps / Score ---
	UFUNCTION(BlueprintCallable, Category = "PowerUp")
	void StartSpeedBoost(float DurationSeconds = 2.5f, float Force = 1800000.f);

	UFUNCTION(BlueprintCallable, Category = "PowerUp")
	void EndSpeedBoost();

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 AddScore(int32 Delta);

	UPROPERTY(EditAnywhere, Category = "PowerUp")
	float BoostForce = 1000.f;

	UPROPERTY(EditAnywhere, Category = "PowerUp")
	float BoostDragScale = 0.6f;

	UPROPERTY(EditAnywhere, Category = "PowerUp")
	float BoostDurationDefault = 2.5f;

	void SetLastCheckpoint(AActor* CheckpointActor) { LastCheckpoint = CheckpointActor; }

	// --- Local HUD events ---
	UPROPERTY(BlueprintAssignable, Category = "HUD")
	FOnLapChangedLocal OnLapChangedLocal;

	UPROPERTY(BlueprintAssignable, Category = "HUD")
	FOnTimeUpdatedLocal OnTimeUpdatedLocal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	float LocalElapsedTime = 0.f;

private:
	// --- Boost internals ---
	bool bBoostActive = false;
	float SavedDragCoefficient = 0.f;
	FTimerHandle BoostTimer;
	int32 Score = 0;

	// --- Crash + Respawn ---
	UFUNCTION()
	void OnCarHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnAnyActorHit(AActor* SelfActor, AActor* OtherActor,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnCrashTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleCarCrash();

	UFUNCTION()
	void RespawnCar();

	UPROPERTY(EditAnywhere, Category = "Crash|Effects")
	UParticleSystem* ExplosionFX = nullptr;

	UPROPERTY(EditAnywhere, Category = "Crash|Settings")
	float CrashForceThreshold = 80000.f;

	UPROPERTY(EditAnywhere, Category = "Crash|Settings")
	float CrashSpeedThreshold = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Crash|Settings")
	float RespawnDelay = 2.0f;

	UPROPERTY()
	bool bIsCrashed = false;

	UPROPERTY()
	AActor* LastCheckpoint = nullptr;

	FVector InitialSpawnLocation;
	FRotator InitialSpawnRotation;

	// --- Respawn / Ghost ---
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnClearRadius = 220.f;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	int32 MaxRespawnSearchSteps = 6;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	float GhostTimeAfterRespawn = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	float LaneOffset = 220.f;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	float ForwardNudgeOnRespawn = 300.f;

	bool bIsGhost = false;

	bool FindSafeRespawnSpot(const FVector& BaseLoc, const FRotator& BaseRot, FVector& OutLoc) const;
	int32 GetRespawnSlot() const;

	void BeginGhost();
	void EndGhost();

	// Optional trigger for overlap detection
	UPROPERTY(EditAnywhere, Category = "Crash|Components")
	class UBoxComponent* CrashTrigger = nullptr;
};
