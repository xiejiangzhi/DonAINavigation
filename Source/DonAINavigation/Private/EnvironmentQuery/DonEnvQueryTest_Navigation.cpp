// Fill out your copyright notice in the Description page of Project Settings.

#include "../DonAINavigationPrivatePCH.h"
#include "DonEnvQueryTest_Navigation.h"
#include "DonNavigationManager.h"
#include "AIController.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"



UDonEnvQueryTest_Navigation::UDonEnvQueryTest_Navigation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	TestPurpose = EEnvTestPurpose::Filter;
	FilterType = EEnvTestFilterType::Match;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	//Context = UEnvQueryContext_Querier::StaticClass();
	SetWorkOnFloatValues(false);
	RandomLocationMaxAttempts = 3;
	RandomLocationRadius = 100.f;
}

void UDonEnvQueryTest_Navigation::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (QueryOwner == nullptr) {
		return;
	}

	const AAIController *AICon = Cast<AAIController>(QueryOwner);
	AActor *ownerActor = nullptr;
	if (AICon) {
		ownerActor = AICon->GetPawn();
	}
	else {
		ownerActor = Cast<AActor>(QueryOwner);
	}
	if (!ownerActor)
		return;

	BoolValue.BindData(QueryOwner, QueryInstance.QueryID);
	bool bWantsValid = BoolValue.GetValue();

	ADonNavigationManager * DonNav = nullptr;

	for (TActorIterator<ADonNavigationManager> It(ownerActor->GetWorld(), ADonNavigationManager::StaticClass()); It; ++It) {
		const ADonNavigationManager *NavMgr = *It;
		if (NavMgr->IsLocationWithinNavigableWorld(ownerActor->GetActorLocation())) {
			DonNav = *It;
			break;
		}
	}

	if (!DonNav)
		return;

	UEnvQueryItemType_Point* ItemTypeCDO = nullptr;
	
	if (bSearchRandomLocation) {
		const auto &itemType = QueryInstance.ItemType;
		if (itemType && itemType->IsChildOf(UEnvQueryItemType_Point::StaticClass())) {
			ItemTypeCDO = QueryInstance.ItemType->GetDefaultObject<UEnvQueryItemType_Point>();
		}
	}

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It) {
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		bool ItemValid = false;
		ItemValid = DonNav->CanNavigate(ItemLocation);
		FVector NavLocation;
		if (bSearchRandomLocation) {
			if (!ItemValid) {
				NavLocation = DonNav->FindRandomPointAroundOriginInNavWorld(ownerActor, ItemLocation, RandomLocationRadius, ItemValid, -1.f, 15.f, RandomLocationMaxAttempts);
			}

			if (ItemTypeCDO) {
				ItemTypeCDO->SetItemNavLocation(It.GetItemData(), FNavLocation(NavLocation));
			}
		}

		It.SetScore(TestPurpose, FilterType, ItemValid, bWantsValid);
	}
}

FText UDonEnvQueryTest_Navigation::GetDescriptionTitle() const
{
	return FText::FromString(FString::Printf(TEXT("Don Navigation: Item is navigable")));
}

FText UDonEnvQueryTest_Navigation::GetDescriptionDetails() const
{
	return FText::Format(FText::FromString("{0}\nDoN Location is navigable"),
		DescribeFloatTestParams());
}




