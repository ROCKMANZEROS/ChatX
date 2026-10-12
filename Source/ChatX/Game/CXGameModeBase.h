#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameModeBase.generated.h"

class ACXPlayerController;

UCLASS()
class CHATX_API ACXGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void OnPostLogin(AController* NewPlayer) override;

	FString GenerateSecretNumber();

	bool IsGuessNumberString(const FString& InNumberString);
	
	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	virtual void BeginPlay() override;

	void PrintCharMessageString(ACXPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	void IncreaseGuessCount(ACXPlayerController* InChattingPlayerController);

	void ResetGame();

	void JudgeGame(ACXPlayerController* InChattingPlayerController, int InStrikeCount);

public:
	// 결과 공지 후 리셋까지 기다릴 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ResetDelay = 5.f;

	// 한 판의 제한 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RoundTime = 30;

private:
	void ReserveReset();

	FTimerHandle ResetTimerHandle;

	bool bIsWaitingForReset = false;

	UFUNCTION()
	void OnMainTimerElapsed();

	FTimerHandle MainTimerHandle;

	void OnTimeOver();

protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ACXPlayerController>> AllPlayerControllers;
};
