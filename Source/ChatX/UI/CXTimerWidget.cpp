#include "UI/CXTimerWidget.h"
#include "Components/TextBlock.h"
#include "Game/CXGameStateBase.h"   // 프로젝트의 실제 include 경로에 맞춰 주세요

void UCXTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsValid(TimerText) == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	ACXGameStateBase* CXGameState = World->GetGameState<ACXGameStateBase>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	TimerText->SetText(FText::AsNumber(CXGameState->RemainTime));
}
