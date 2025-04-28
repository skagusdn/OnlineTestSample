// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSamplePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ONLINETESTSAMPLE_API AOnlineSamplePlayerController : public APlayerController
{
	GENERATED_BODY()

public :
	AOnlineSamplePlayerController();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
