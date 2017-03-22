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

#include "GameFramework/Actor.h"
#include "Legacy/DEPRECATED_DoNNavigationVolumeComponent.h"
#include "DonNavigationCommon.h"

#include "DEPRECATED_VolumeAdaptiveBuilder.generated.h"

USTRUCT()
struct FNavigationGraphAerial {
	
	GENERATED_USTRUCT_BODY()	
	
	// Q:How does GC work for these nodes? 
	// A: It doesn't work if these volumes are persisted onto the UMAP and loaded; in which case, these nodes are rehydrated from the property 'NavGraph_GCSafe'
	//UPROPERTY()
	TMap<UDoNNavigationVolumeComponent*, TArray<UDoNNavigationVolumeComponent*>> nodes;	

	inline const TArray<UDoNNavigationVolumeComponent*> neighbors(UDoNNavigationVolumeComponent* id)
	{
		TArray<UDoNNavigationVolumeComponent*>* var = nodes.Find(id);
		if (var)
			return *var;
		else
			return TArray<UDoNNavigationVolumeComponent*>();		
	}	

	FVector locationVectorValue(UDoNNavigationVolumeComponent* a) {
		return a->GetComponentLocation();
	}
};

USTRUCT()
struct FNAVMapContainer {

	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UDoNNavigationVolumeComponent* volume;

	UPROPERTY()	
	TArray<UDoNNavigationVolumeComponent*> neighbors;
};

/**
*
*/
UCLASS(Deprecated)
class ADEPRECATED_VolumeAdaptiveBuilder : public AActor
{
	GENERATED_BODY()

public:	
	ADEPRECATED_VolumeAdaptiveBuilder(const FObjectInitializer& ObjectInitializer);

	void DisplayGraphDebugInfo();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AerialNavigation")
	FNavigationGraphAerial NavGraph;	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AerialNavigation")
	TArray<FNAVMapContainer> NavGraph_GCSafe;

	//
		
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Translation)
	USceneComponent* SceneComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Translation)
	UBillboardComponent* Billboard;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Translation)
	TArray<UDoNNavigationVolumeComponent*> NAVVolumeComponents;	

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PixelDimensions)
	//FNAVSeedXyz NAVSeedData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ParticleSystem)
	UParticleSystem* SeedVisualizer;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PixelDimensions)
	//FNAVSeedXyz NAVVolumeData;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PixelDimensions)
	float XBaseUnit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PixelDimensions)
	float YBaseUnit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PixelDimensions)
	float ZBaseUnit;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PixelDimensions)
	//float OverlapThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PixelDimensions)
	float OffsetThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PixelDimensions)
	float FloorClearance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldDimensinos)
	int32 XGridSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldDimensinos)
	int32 YGridSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldDimensinos)
	int32 ZGridSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldDimensinos)
	int32 XYAspectRatioThreshold;  /* wrong use of "threshold" here :P */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WorldDimensinos)
	bool UseAspectRatioThreshold;  /* wrong use of "threshold" here :P */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool GenerateNavigationVolumes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool RegenerateNAVNetwork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool CleanUpAllData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool DisplayNAVNeighborGraph;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool IsVisibleNavigableVolumes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool IsVisibleBlockedVolumes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visualization)
	bool IsVisibleInGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObstacleDetection)
	TArray<TEnumAsByte<EObjectTypeQuery>> ObstacleList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObstacleDetection)
	TArray<TEnumAsByte<EObjectTypeQuery>> NAVOverlapQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObstacleDetection)
	TArray<AActor*> ActorsToIgnoreForCollision;

	UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	void ConstructBuilder();

	UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	void GenerateAdaptiveNavigationVolumeSeeds();	

	UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	bool GrowNAVVolumeByIndex(UDoNNavigationVolumeComponent* volume, int32 XGrowth, int32 YGrowth, int32 ZGrowth);
	
	void GrowNAVVolumeToCapacity(UDoNNavigationVolumeComponent* volume, std::vector< std::vector<int> > &seedTrackerZ, std::vector< std::vector< std::vector<bool> > > &SeedTrackerXYZ);

	UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	UDoNNavigationVolumeComponent* CreateNAVVolume(FVector Location, FName VolumeName, int32 SeedX, int32 SeedY, int32 SeedZ );

	UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	void CleanUp();

	UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	void BuildNAVNetwork();	

	//UFUNCTION(BlueprintCallable, Category = NAVBuilder)
	//bool IsNAVIndexValid(int32 x, int32 y, int32 z);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;	
#endif // WITH_EDITOR	
	
	////////////////////////////////////////////////////////////////////////////////////////
	
	// Core shortest path algorithms
		
	/* Returns path solution using a dense network of UDoNNavigationVolumeComponent. Maps using the Adaptive Navigation Builder should use this function.*/
	UFUNCTION(BlueprintCallable, Category = "AerialNavigation", meta = (DeprecatedFunction, DeprecationMessage = "This function is deprecated, please use FindPathSolution instead."))
	TArray<UDoNNavigationVolumeComponent*> GetShortestPathToDestination(FVector origin, FVector destination, UDoNNavigationVolumeComponent* originVolume, UDoNNavigationVolumeComponent* destinationVolume, TArray<FVector> &PathSolutionRaw, TArray<FVector> &PathSolutionOptimized, bool DrawDebugVolumes, bool VisualizeRawPath, bool VisualizeOptimizedPath, bool VisualizeInRealTime, float LineThickness);

	UFUNCTION(BlueprintCallable, Category = "AerialNavigation")
	void GetShortestPathToDestination_DebugRealtime(bool Reset, FVector origin, FVector destination, UDoNNavigationVolumeComponent* originVolume, UDoNNavigationVolumeComponent* destinationVolume, bool DrawDebug, TArray<FVector> &PathSolutionRaw, TArray<FVector> &PathSolutionOptimized, bool DrawDebugVolumes);

	void ExpandFrontierTowardsTarget(UDoNNavigationVolumeComponent* current, UDoNNavigationVolumeComponent* neighbor, DoNNavigation::PriorityQueue<UDoNNavigationVolumeComponent*> &frontier, TMap<UDoNNavigationVolumeComponent*, FVector> &entryPointMap, bool &goalFound, UDoNNavigationVolumeComponent* start, UDoNNavigationVolumeComponent* goal, FVector origin, FVector destination, TMap<UDoNNavigationVolumeComponent*, int>& VolumeVsCostMap, bool DrawDebug, TMap<UDoNNavigationVolumeComponent*, TArray<UDoNNavigationVolumeComponent*>> &PathVolumeSolutionMap);

	UFUNCTION(BlueprintPure, Category = "AerialNavigation")
	FVector NavEntryPointsForTraversal(FVector Origin, UDoNNavigationVolumeComponent* CurrentVolume, UDoNNavigationVolumeComponent* DestinationVolume, float &SegmentDist, bool DrawDebug);	

	////////////////////////////////////////////////////////////////////////////////////////	
	
	TArray<FVector> PathSolutionFromVolumeSolution(TArray<UDoNNavigationVolumeComponent*> VolumeSolution, FVector Origin, FVector Destination, bool DrawDebugVolumes);
	
	void OptimizePathSolution(const TArray<FVector>& PathSolution, TArray<FVector> &PathSolutionOptimized);
	
	void OptimizePathSolution_Pass1_LineTrace(const TArray<FVector>& PathSolution, TArray<FVector> &PathSolutionOptimized);

	bool IsDirectPath(FVector start, FVector end, FHitResult &OutHit);

	////////////////////////////////////////////////////////////////////////////////////////	

	// Navigation traversal functions (for Adapive Builder only)
	
	UFUNCTION(BlueprintPure, Category = "AerialNavigation")
	FVector NavigaitonEntryPointFromVector(FVector Origin, UDoNNavigationVolumeComponent* CurrentVolume, UDoNNavigationVolumeComponent* DestinationVolume, bool &VolumeTraversalImminent, bool &overlapsX, bool &overlapsY, bool &overlapsZ, bool DrawDebug);

	UFUNCTION(BlueprintPure, Category = "AerialNavigation")
	FVector NavigaitonEntryPoint(AActor* Actor, UDoNNavigationVolumeComponent* CurrentVolume, UDoNNavigationVolumeComponent* DestinationVolume, bool &VolumeTraversalImminent, bool &overlapsX, bool &overlapsY, bool &overlapsZ);	

	UFUNCTION(BlueprintPure, Category = "AerialNavigation")
	FVector NavEntryPointFromPath(FVector Origin, FVector FinalDestination, int32 currentVolumeIndex, TArray<UDoNNavigationVolumeComponent*> path, bool &VolumeTraversalImminent, int32 &newVolumeIndex, bool DrawDebugVolumes);

	////////////////////////////////////////////////////////////////////////////////////////

	// NAV Visualizer

	UFUNCTION(BlueprintCallable, Category = "AerialNavigation")
	void VisualizeNAVResult(const TArray<FVector>& PathSolution, FVector Source, FVector Destination, bool Reset, bool DrawDebugVolumes, FColor LineColor, float LineThickness);

	UFUNCTION(BlueprintCallable, Category = "AerialNavigation")
	void VisualizeNAVResultRealTime(const TArray<FVector>& PathSolution, FVector Source, FVector Destination, bool Reset, bool DrawDebugVolumes, FColor LineColor, float LineThickness);

	UFUNCTION(BlueprintCallable, Category = "AerialNavigation")
	void VisualizeSolution(FVector source, FVector destination, const TArray<FVector>& PathSolutionRaw, const TArray<FVector>& PathSolutionOptimized, bool VisualizeRawPath, bool VisualizeOptimizedPath, bool VisualizeInRealTime, bool DrawDebugVolumes, float LineThickness);

	////////////////////////////////////////////////////////////////////////////////////////

	// Utility functions
	UFUNCTION(BlueprintPure, Category = "AerialNavigation")
	UDoNNavigationVolumeComponent* GetNAVVolumeFromObject(UObject* Actor);

	UFUNCTION(BlueprintPure, Category = "AerialNavigation")
	UDoNNavigationVolumeComponent* GetNAVVolumeFromComponent(UPrimitiveComponent* Component, FVector ComponentAt);

	////////////////////////////////////////////////////////////////////////////////////////

};