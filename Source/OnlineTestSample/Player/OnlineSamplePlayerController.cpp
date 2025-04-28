// Fill out your copyright notice in the Description page of Project Settings.


#include "OnlineSamplePlayerController.h"

#include "OnlineTestSample/GameInstance/OnlineSampleGameInstance.h"
#include "OnlineTestSample/GameInstance/OnlineSampleOnlineSubsystem.h"

AOnlineSamplePlayerController::AOnlineSamplePlayerController()
{
 
}
 

void AOnlineSamplePlayerController::BeginPlay()
{
	Super::BeginPlay();

	UOnlineSampleGameInstance* GameInstance = Cast<UOnlineSampleGameInstance>(GetGameInstance());
	UOnlineSampleOnlineSubsystem* OnlineSubsystem = GameInstance->GetSubsystem<UOnlineSampleOnlineSubsystem>();
	ULocalPlayer* LocalPlayer = Super::GetLocalPlayer();

	if(LocalPlayer)
	{
		FPlatformUserId LocalPlayerPlatformUserId = LocalPlayer->GetPlatformUserId();
				
		if(OnlineSubsystem)
		{
			UE_LOG(LogOnlineSampleOnlineSubsystem, Log, TEXT("Registering PlatformUserId: %d"), LocalPlayerPlatformUserId.GetInternalId());
			// OnlineSubsystem->RegisterLocalOnlineUser(LocalPlayerPlatformUserId);
			
			//로그인 부분 임시.
			OnlineSubsystem->Login(LocalPlayerPlatformUserId);
			
			
			// // 타이틀 파일을 읽고 화면에 콘텐츠를 표시합니다
			// FString TitleFileContent = OnlineSubsystem->ReadTitleFile(FString("StatusFile"), LocalPlayerPlatformUserId);
			// if (GEngine)
			// {
			// 	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Black, TitleFileContent);
			// }
		}
	}
}

void AOnlineSamplePlayerController::EndPlay(EEndPlayReason::Type EndReason)
{
	Super::EndPlay(EndReason);
}