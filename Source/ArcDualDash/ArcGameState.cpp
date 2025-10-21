#include "ArcGameState.h"
#include "Engine/World.h"
#include "TimerManager.h"

void AArcGameState::BeginPlay()
{
    Super::BeginPlay();

    // notes: start counting from zero, fire every 1s
    ElapsedSeconds = 0;

    GetWorldTimerManager().SetTimer(
        TimerHandle_CountUp,
        this,
        &AArcGameState::TickOneSecond,
        1.0f,   // rate
        true,   // looping
        1.0f    // first delay (start at 00:01 after 1 second)
    );

    UE_LOG(LogTemp, Log, TEXT("[Timer] Started count-up from 00:00"));
}

void AArcGameState::TickOneSecond()
{
    ++ElapsedSeconds;

    const int32 Minutes = ElapsedSeconds / 60;
    const int32 Seconds = ElapsedSeconds % 60;

    // notes: use Log (not Verbose) so it's visible in Output Log by default
    UE_LOG(LogTemp, Log, TEXT("[Timer] %02d:%02d"), Minutes, Seconds);

    // notes: small on-screen debug so I SEE it incrementing
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            /*Key=*/ 12345,      // constant key – overwrites previous line
            /*TimeToDisplay=*/ 1.1f,
            FColor::White,
            FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)
        );
    }
}

