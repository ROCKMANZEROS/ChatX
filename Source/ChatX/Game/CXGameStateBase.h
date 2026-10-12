#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameStateBase.generated.h"

UCLASS()
class CHATX_API ACXGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastLoginMessage(const FString& InNameString = FString(TEXT("XXXXXXXX")));

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버가 관리하고 모든 클라이언트에 복제되는 남은 시간(초)
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 RemainTime = 0;
};
