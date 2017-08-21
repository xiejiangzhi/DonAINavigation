// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "DonEnvQueryTest_Navigation.generated.h"

/**
 * 
 */
UCLASS()
class DONAINAVIGATION_API UDonEnvQueryTest_Navigation : public UEnvQueryTest
{
	GENERATED_BODY()
	
public:
	UDonEnvQueryTest_Navigation(const FObjectInitializer& ObjectInitializer);
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;


public:
	/**  */
	//UPROPERTY(EditDefaultsOnly, Category = Trace)
	//TSubclassOf<UEnvQueryContext> Context;

	/** Search for random location nearby */
	UPROPERTY(EditDefaultsOnly, Category = Test, meta = (InlineEditConditionToggle))
	uint32 bSearchRandomLocation : 1;

	/** Number of Attempts to find random location nearby if Item's location is not valid */
	UPROPERTY(EditDefaultsOnly, Category = Test, meta = (EditCondition = "bSearchRandomLocation"))
	int32 RandomLocationMaxAttempts;

	/** Find Random location radius */
	UPROPERTY(EditDefaultsOnly, Category = Test, meta = (EditCondition = "bSearchRandomLocation"))
	float RandomLocationRadius;

private:
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;	
	
};
