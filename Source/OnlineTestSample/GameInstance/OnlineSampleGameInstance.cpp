// Fill out your copyright notice in the Description page of Project Settings.


#include "OnlineSampleGameInstance.h"

#include "OnlineSampleOnlineSubsystem.h"
#include "OnlineTestSample/Player/OnlineSamplePlayerController.h"

DEFINE_LOG_CATEGORY(LogGameInstance);

void UOnlineSampleGameInstance::Init()
{
	UE_LOG(LogGameInstance, Log, TEXT("OnlineSampleGameInstance initialized."));
	Super::Init();
}

void UOnlineSampleGameInstance::Shutdown()
{
	UE_LOG(LogGameInstance, Log, TEXT("OnlineSampleGameInstance shutdown."));
	Super::Shutdown();
}


UOnlineSampleGameInstance::UOnlineSampleGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

AOnlineSamplePlayerController* UOnlineSampleGameInstance::GetPrimaryPlayerController() const
{
	return Cast<AOnlineSamplePlayerController>(Super::GetPrimaryPlayerController(false));
}

