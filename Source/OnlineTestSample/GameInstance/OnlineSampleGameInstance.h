// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSampleGameInstance.generated.h"

class AOnlineSamplePlayerController;
DECLARE_LOG_CATEGORY_EXTERN(LogGameInstance, Log, All);

/**
 * 
 */
UCLASS()
class ONLINETESTSAMPLE_API UOnlineSampleGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

public:

	
	UOnlineSampleGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	AOnlineSamplePlayerController* GetPrimaryPlayerController() const;

	void CreateSession();
	
};
