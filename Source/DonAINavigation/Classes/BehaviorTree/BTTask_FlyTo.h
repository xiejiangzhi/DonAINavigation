// The MIT License(MIT)
//
// Copyright(c) 2015 Venugopalan Sreedharan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

#include "DonNavigationHelper.h"

#include "BTTask_FlyTo.generated.h"

USTRUCT()
struct FBT_FlyToTarget_DebugParams : public FDoNNavigationDebugParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	bool bVisualizePawnAsVoxels;
};


struct FBT_FlyToTarget_Metadata
{
	uint16 ActiveInstanceIdx;

	TWeakObjectPtr<class UBehaviorTreeComponent> OwnerComp;	

	FBT_FlyToTarget_Metadata(){}

	FBT_FlyToTarget_Metadata(uint16 ActiveInstanceIdx, UBehaviorTreeComponent* OwnerComp) : ActiveInstanceIdx(ActiveInstanceIdx), OwnerComp(OwnerComp){}

};


struct FBT_FlyToTarget
{	
	FBT_FlyToTarget_Metadata Metadata;

	FDoNNavigationQueryParams QueryParams;

	FDonNavigationDynamicCollisionDelegate DynamicCollisionListener;

	int32 solutionTraversalIndex = 0;

	FDoNNavigationQueryData QueryResults;

	bool bSolutionInvalidatedByDynamicObstacle = false;	

	bool bIsANavigator = false;

	void Reset()
	{	
		solutionTraversalIndex = 0;
		QueryResults = FDoNNavigationQueryData();
		QueryParams = FDoNNavigationQueryParams();
		Metadata = FBT_FlyToTarget_Metadata();
		bSolutionInvalidatedByDynamicObstacle = false;
	}
};

/**
 * 
 */
UCLASS()
class UBTTask_FlyTo : public UBTTaskNode
{
	GENERATED_BODY()

	public:
	UBTTask_FlyTo(const FObjectInitializer& ObjectInitializer);	
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	void HandleTaskFailure(UBlackboardComponent* blackboard);

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

	// Behavior Tree Input:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	FBlackboardKeySelector FlightLocationKey;

	/* Optional: Useful in somecases where you want failure or success of a task to automatically update a particular blackboard key*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	FBlackboardKeySelector FlightResultKey;

	/* Optional: This boolean will be flip-flopped at the end of this task (regardless of success or failure). This can be useful for certain types of behavior tree setups*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	FBlackboardKeySelector KeyToFlipFlopWhenTaskExits;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	float MinimumProximityRequired = 15.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	FDoNNavigationQueryParams QueryParams;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	FBT_FlyToTarget_DebugParams DebugParams;

	UFUNCTION(BlueprintCallable, Category="DoN Navigation")
	void Pathfinding_OnFinish(const FDoNNavigationQueryData& Data);

	UFUNCTION(BlueprintCallable, Category="DoN Navigation")
	void Pathfinding_OnDynamicCollisionAlert(const FDonNavigationDynamicCollisionPayload& Data);	

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	ADonNavigationManager* NavigationManager;


protected:

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	FBT_FlyToTarget* TaskMemoryFromGenericPayload(void* GenericPayload);

	EBTNodeResult::Type SchedulePathfindingRequest(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);	

	void AbortPathfindingRequest(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

	void TickPathNavigation(UBehaviorTreeComponent& OwnerComp, FBT_FlyToTarget* MyMemory, float DeltaSeconds);
};