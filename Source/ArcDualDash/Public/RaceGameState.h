#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RaceGameState.generated.h"

// Forward declarations
class AMyCar;
class ACheckpoints;

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdated, float, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLapChanged, int32, NewLap, int32, TotalLaps);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeaderboardUpdated);

// Struct for race data
USTRUCT(BlueprintType)
struct FPlayerRaceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	AMyCar* Car = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	int32 Lap = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Checkpoint = 0;

	// Combined progress key (laps + checkpoints + fractional segment)
	UPROPERTY(BlueprintReadWrite)
	float ProgressKey = 0.f;

	// Distance to next checkpoint (used as tie-breaker)
	UPROPERTY(BlueprintReadWrite)
	float DistanceToNext = 0.f;
};

UCLASS()
class ARCDUALDASH_API ARaceGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ARaceGameState();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Delegates for UI ---
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTimeUpdated OnTimeUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLapChanged OnLapChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLeaderboardUpdated OnLeaderboardUpdated;

	// --- Race data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
	int32 TotalLaps = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	int32 CurrentLap = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	float ElapsedTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
	bool bTimerRunning = true;

	// --- Leaderboard ---
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	TArray<FPlayerRaceData> Leaderboard;

	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void UpdateLeaderboard();

	UFUNCTION(BlueprintCallable, Category = "Race")
	void IncrementLapAndBroadcast();

	UFUNCTION(BlueprintCallable, Category = "Race")
	void ResetTimer();

	UFUNCTION(BlueprintCallable, Category = "Race")
	void StopTimer();

private:
	FTimerHandle LeaderboardTimerHandle;

	// Cached checkpoints
	UPROPERTY()
	TArray<AActor*> TrackCheckpoints;

	int32 NumCheckpoints = 0;

	// Helper function for fractional progress along checkpoint segment
	static float CalculateSegmentT(const FVector& A, const FVector& B, const FVector& P)
	{
		FVector AB = B - A;
		const float Len2 = FMath::Max(AB.SizeSquared(), 1.f);
		float T = FVector::DotProduct(P - A, AB) / Len2;
		return FMath::Clamp(T, 0.f, 1.f);
	}
};
