#include "CXGameModeBase.h"
#include "Game/CXGameModeBase.h"
#include "CXGameStateBase.h"
#include "Player/CXPlayerController.h"
#include "Player/CXPlayerState.h"
#include "EngineUtils.h"

void ACXGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(NewPlayer);
	if (IsValid(CXPlayerController) == true)
	{
		CXPlayerController->NotificationText = FText::FromString(TEXT("Connected to the game server."));

		AllPlayerControllers.Add(CXPlayerController);

		ACXPlayerState* CXPS = CXPlayerController->GetPlayerState<ACXPlayerState>();
		if (IsValid(CXPS) == true)
		{
			CXPS->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());
		}

		ACXGameStateBase* CXGameStateBase = GetGameState<ACXGameStateBase>();
		if (IsValid(CXGameStateBase) == true)
		{
			CXGameStateBase->MulticastRPCBroadcastLoginMessage(CXPS->PlayerNameString);
		}
	}
}

FString ACXGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());
	// Numbers = Numbers.FilterByPredicate([](int32 Num) { return Num > 0; });

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool ACXGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	bool bCanPlay = false;

	// 3자리가 넘는지 확인
	do {
		if (InNumberString.Len() != 3)
		{
			break;
		}

		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			// 문자 확인
			if (FChar::IsDigit(C) == false || C == '0')
			{
				bIsUnique = false;
				break;
			}

			UniqueDigits.Add(C);
		}

		// 서로 다른 숫자가 3개가 아니면 중복이 있는 것
		if (UniqueDigits.Num() != 3)
		{
			bIsUnique = false;
		}

		// 중복 확인
		if (bIsUnique == false)
		{
			break;
		}

		bCanPlay = true;

	} while (false);

	return bCanPlay;
}

FString ACXGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ACXGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();

	// 남은 시간 초기화와 1초마다 도는 타이머 시작
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	if (IsValid(CXGameState) == true)
	{
		CXGameState->RemainTime = RoundTime;
	}

	GetWorldTimerManager().SetTimer(MainTimerHandle, this, &ThisClass::OnMainTimerElapsed, 1.f, true);
}

void ACXGameModeBase::PrintCharMessageString(ACXPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	// 메시지 전체가 숫자로만 되어 있는가 (빈 메시지는 제외)
	bool bIsAllDigits = (InChatMessageString.IsEmpty() == false);
	for (TCHAR C : InChatMessageString)
	{
		if (FChar::IsDigit(C) == false)
		{
			bIsAllDigits = false;
			break;
		}
	}

	bool bIsGuessAttempt = bIsAllDigits;   // 숫자만 쳤으면 숫자 입력 시도
	bool bIsGuess = false;                 // 그 입력이 규칙에 맞는가
	FString GuessNumberString = InChatMessageString;

	if (bIsGuessAttempt == true)
	{
		bIsGuess = IsGuessNumberString(GuessNumberString);
	}

	// 숫자 입력을 시도했는데 규칙에 안 맞으면 보낸 사람에게만 안내하고 끝낸다 (기회 소진 X)
	if (bIsGuessAttempt == true && bIsGuess == false)
	{
		InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("Invalid input. Please enter again. (example: 123)"));
		return;
	}

	// 결과 공지 후 리셋을 기다리는 동안에는 숫자 입력을 받지 않는다
	if (bIsGuess == true && bIsWaitingForReset == true)
	{
		InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("The game is over. A new game will start soon."));
		return;
	}

	// 이미 기회를 다 쓴 플레이어는 숫자 입력을 받지 않는다
	if (bIsGuess == true)
	{
		ACXPlayerState* SenderPlayerState = InChattingPlayerController->GetPlayerState<ACXPlayerState>();
		if (IsValid(SenderPlayerState) == true && SenderPlayerState->CurrentGuessCount >= SenderPlayerState->MaxGuessCount)
		{
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("No chances left. Please wait for the other players."));
			return;
		}
	}

	// 1. 숫자 입력이면 시도 횟수를 먼저 올린다 (서버 값이 기준)
	if (bIsGuess == true)
	{
		IncreaseGuessCount(InChattingPlayerController);
	}

	// 2. 올라간 값으로 "이름(현재/최대): " 접두어를 서버에서 만든다
	FString PrefixString;
	ACXPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ACXPlayerState>();
	if (IsValid(CXPS) == true)
	{
		PrefixString = CXPS->GetPlayerInfoString() + TEXT(": ");
	}

	FString CombinedMessageString = PrefixString + InChatMessageString;

	if (bIsGuess == true)
	{
		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);
		CombinedMessageString = CombinedMessageString + TEXT(" -> ") + JudgeResultString;

		for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
		{
			ACXPlayerController* CXPlayerController = *It;
			if (IsValid(CXPlayerController) == true)
			{
				CXPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
			}
		}

		int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
		JudgeGame(InChattingPlayerController, StrikeCount);

	}
	else
	{
		for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
		{
			ACXPlayerController* CXPlayerController = *It;
			if (IsValid(CXPlayerController) == true)
			{
				CXPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
			}
		}
	}
}

void ACXGameModeBase::IncreaseGuessCount(ACXPlayerController* InChattingPlayerController)
{
	ACXPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ACXPlayerState>();
	if (IsValid(CXPS) == true)
	{
		CXPS->CurrentGuessCount++;
	}
}

void ACXGameModeBase::ResetGame()
{
	bIsWaitingForReset = false;

	SecretNumberString = GenerateSecretNumber();

	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	if (IsValid(CXGameState) == true)
	{
		CXGameState->RemainTime = RoundTime;
	}

	for (const auto& CXPlayerController : AllPlayerControllers)
	{
		CXPlayerController->NotificationText = FText::GetEmpty();   // 결과 공지 지우기

		ACXPlayerState* CXPS = CXPlayerController->GetPlayerState<ACXPlayerState>();
		if (IsValid(CXPS) == true)
		{
			CXPS->CurrentGuessCount = 0;
		}
	}
}

void ACXGameModeBase::JudgeGame(ACXPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (3 == InStrikeCount)
	{
		ACXPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ACXPlayerState>();
		for (const auto& CXPlayerController : AllPlayerControllers)
		{
			if (IsValid(CXPS) == true)
			{
				FString CombinedMessageString = CXPS->PlayerNameString + TEXT(" has won the game.");
				CXPlayerController->NotificationText = FText::FromString(CombinedMessageString);
			}

			ReserveReset();
		}
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& CXPlayerController : AllPlayerControllers)
		{
			ACXPlayerState* CXPS = CXPlayerController->GetPlayerState<ACXPlayerState>();
			if (IsValid(CXPS) == true)
			{
				if (CXPS->CurrentGuessCount < CXPS->MaxGuessCount)
				{
					bIsDraw = false;
					break;
				}
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& CXPlayerController : AllPlayerControllers)
			{
				CXPlayerController->NotificationText = FText::FromString(TEXT("Draw..."));
			}

			ReserveReset();
		}
	}
}

void ACXGameModeBase::ReserveReset()
{
	bIsWaitingForReset = true;
	GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &ThisClass::ResetGame, ResetDelay, false);
}

void ACXGameModeBase::OnMainTimerElapsed()
{
	ACXGameStateBase* CXGameState = GetGameState<ACXGameStateBase>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	// 결과 공지 후 리셋을 기다리는 동안은 시간을 줄이지 않는다
	if (bIsWaitingForReset == true)
	{
		return;
	}

	if (CXGameState->RemainTime > 0)
	{
		CXGameState->RemainTime = CXGameState->RemainTime - 1;
		// UE_LOG(LogTemp, Warning, TEXT("RemainTime: %d"), CXGameState->RemainTime);   // 확인용

		// 방금 0이 된 순간에만 한 번 호출된다
		if (CXGameState->RemainTime == 0)
		{
			OnTimeOver();
		}
	}

	
}

void ACXGameModeBase::OnTimeOver()
{
	// 승리/무승부 공지와 같은 방식으로 모든 플레이어에게 알린다
	// (JudgeGame에서 쓰는 컨트롤러 목록과 같은 방식으로 맞춰 주세요)
	for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
	{
		ACXPlayerController* CXPlayerController = *It;
		if (IsValid(CXPlayerController) == true)
		{
			CXPlayerController->NotificationText = FText::FromString(TEXT("Time over. A new game will start soon."));
		}
	}

	ReserveReset();
}


