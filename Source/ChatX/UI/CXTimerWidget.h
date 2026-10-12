#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXTimerWidget.generated.h"

class UTextBlock;

UCLASS()
class CHATX_API UCXTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// 위젯 블루프린트의 TextBlock 이름이 TimerText와 정확히 같아야 한다
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimerText;
};