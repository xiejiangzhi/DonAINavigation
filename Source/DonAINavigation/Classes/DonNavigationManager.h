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

#include "DonNavigationCommon.h"
#include "Multithreading/DonDrawDebugThreadSafe.h"

#include "DonNavigationManager.generated.h"

DECLARE_STATS_GROUP(TEXT("DonNavigation"), STATGROUP_DonNavigation, STATCAT_Advanced);

UENUM()
enum class EDonNavigationQueryStatus : uint8
{	
	Unscheduled,
	InProgress,
	Success,
	Failure,
	QueryHasNoSolution,
	TimedOut
};

struct FDonNavigationDynamicCollisionNotifyee;

/**
*
*/
USTRUCT()
struct FDonNavigationVoxel
{
	GENERATED_USTRUCT_BODY()	
	
	int32 X;

	int32 Y;

	int32 Z;

	FVector Location;

	uint8 NumResidents = 0;

	bool bIsInitialized = false;
	
	TArray<FDonNavigationDynamicCollisionNotifyee> DynamicCollisionNotifyees;

	bool FORCEINLINE CanNavigate() { return NumResidents == 0; }	

	void SetNavigability(bool CanNavigate)
	{
		if (!CanNavigate)
			NumResidents++;
		else
			NumResidents = NumResidents > 0 ? NumResidents - 1 : 0;
	}
	
	void BroadcastCollisionUpdates();
	
	friend bool operator== (const FDonNavigationVoxel& A, const FDonNavigationVoxel& B)
	{
		return A.X == B.X && A.Y == B.Y && A.Z == B.Z;
	}

	FDonNavigationVoxel(){}	
};

USTRUCT()
struct FDonVoxelCollisionProfile
{
	GENERATED_USTRUCT_BODY()
	
	TArray<FVector> RelativeVoxelOccupancy;	

	/** 
	* Note:- These references are only valid so long as NAVVolumeData (see ADonNavigationManager) is not modified in any way.
	* Presently, NAVVolumeData is populated once and only once (at the beginning of the game) so these referenfecs _should_ be safe to rely upon.
	*/	

	TArray<FDonNavigationVoxel*> WorldVoxelsOccupied;

};

USTRUCT()
struct FDonNavigationDynamicCollisionPayload
{
	GENERATED_USTRUCT_BODY()
	
	void* CustomDelegatePayload;	
	
	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	FDonNavigationVoxel Voxel;

	FDonNavigationDynamicCollisionPayload(){}

	FDonNavigationDynamicCollisionPayload(void* CustomDelegatePayload, FDonNavigationVoxel Voxel) : CustomDelegatePayload(CustomDelegatePayload), Voxel(Voxel){}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FDonNavigationDynamicCollisionDelegate, const FDonNavigationDynamicCollisionPayload&, Data); // note: non-dynamic delegate can't be used as a function parameter apparently

USTRUCT()
struct FDonNavigationDynamicCollisionNotifyee
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FDonNavigationDynamicCollisionDelegate Listener;

	FDonNavigationDynamicCollisionPayload Payload;

	FDonNavigationDynamicCollisionNotifyee(){}

	FDonNavigationDynamicCollisionNotifyee(FDonNavigationDynamicCollisionDelegate Listener, FDonNavigationDynamicCollisionPayload Payload) : Listener(Listener), Payload(Payload)
	{

	}

	friend bool operator== (const FDonNavigationDynamicCollisionNotifyee& A, const FDonNavigationDynamicCollisionNotifyee& B)
	{
		return A.Listener == B.Listener;
	}
};

// @todo: convert to regular struct and profile startup speed. Usinng a USTRUCT for these may be excessive

USTRUCT()
struct FDonNavVoxelY
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FDonNavigationVoxel> Z;	

	void AddZ(FDonNavigationVoxel volume)
	{
		Z.Add(volume);
	}

	FDonNavVoxelY()
	{
	}
};


USTRUCT()
struct FDonNavVoxelX
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FDonNavVoxelY> Y;	

	void AddY(FDonNavVoxelY YPlaneVolume)
	{
		Y.Add(YPlaneVolume);
	}

	FDonNavVoxelX()
	{
	}
};

USTRUCT()
struct FDonNavVoxelXYZ
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FDonNavVoxelX> X;	

	void AddX(FDonNavVoxelX XPlaneVolume)
	{
		X.Add(XPlaneVolume);
	}

	void ClearAll()
	{
		for (FDonNavVoxelX _x : X)
		{
			for (FDonNavVoxelY _y : _x.Y)
				_y.Z.Empty();

			_x.Y.Empty();
		}

		X.Empty();
	}

	FDonNavVoxelXYZ()
	{
	}	
};

USTRUCT()
struct FDoNNavigationQueryParams
{
	GENERATED_USTRUCT_BODY()

	/** If a query takes more time to run than the value specified here, the pathfinding task will abort
	*   This is useful to prevent expensive queries (eg: by passing a destination for which no solution exists)
	*   from clogging up the pathfinding system
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	float QueryTimeout = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	bool bFlexibleOriginGoal = true;

	/** Skips optimization of the path solution completely. Optimized paths are shorter and more visually cogent
	*   but come at a cost. Maps with low voxel density (high VoxelSize value) usually need optimization for best results
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	bool bSkipOptimizationPass = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	int32 MaxOptimizerSweepAttemptsPerNode = 25;

	/** Enabling this will sample all voxels of your pawn or character for determining whether a path solution
	*  needs to be recalculated due to dynamic obstacles. This will improve the accuracy of response to dynamic collisions
	*  but comes at a steep cost as the number of event delegates required for listening to precise dynamic collisions is high
	*  Only use this if you really need to.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	bool bPreciseDynamicCollisionRepathing = false;

	/** Allows you to inflate your mesh's collision extents by a fixed increment for all sweep based testing.
	*   Typically used if you find your mesh bumping into obstacles while navigating along path solutions.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	float CollisionShapeInflation = 0.f;	

	/** By default the navigation manager will not allow you to schedule a new query if a pre-existing query
	*   scheduled by you is not yet complete. You can override this behavior by forcing the manager to abort
	*   the old query and reschedule this one. Use with discretion: Hammering the manager with queries can
	*   quickly degrade performance (especially in a dynamic collision context).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	bool bForceRescheduleQuery = false;	

	/** Generic pointer allowing you to store anything you like to be passed back as payload.
	*   Typically used for passing unqiue identifiers in situations where you can't otherwise identify the task owner
	*   (Eg: Behavior tree singleton nodes)
	*/
	void* CustomDelegatePayload = NULL;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoN Navigation")
	bool bTemp_WalkSolution = false;

	FDoNNavigationQueryParams()
	{

	}	
};


USTRUCT()
struct FDoNNavigationDebugParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	bool DrawDebugVolumes = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	bool VisualizeRawPath = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	bool VisualizeOptimizedPath = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	bool VisualizeInRealTime = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	float LineThickness = 2.f;

	/* -1 signifies persistent lines that need to be flushed out manually to clear them*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DoN Navigation")
	float LineDuration = -1.f;

	FDoNNavigationDebugParams() {}

	FDoNNavigationDebugParams(bool _DrawDebugVolumes, bool _VisualizeRawPath, bool _VisualizeOptimizedPath, bool _VisualizeInRealTime, float _LineThickness)
	{
		DrawDebugVolumes = _DrawDebugVolumes;
		VisualizeRawPath = _VisualizeRawPath;
		VisualizeOptimizedPath = _VisualizeOptimizedPath;
		VisualizeInRealTime = _VisualizeInRealTime;
		LineThickness = _LineThickness;
	}
};

struct FLocVector : public FVector
{
	FLocVector(){}

	FLocVector(FVector Vector)
	{
		X = Vector.X;
		Y = Vector.Y;
		Z = Vector.Z;
	}

	friend bool operator< (const FLocVector& A, const FLocVector& B)
	{
		// This function is simply a workaround for enabling use of FVector type with priority queue.
		// The value returned is irrelevant:

		return false;
	}
};

USTRUCT(BlueprintType)
struct FDoNNavigationQueryData
{
	GENERATED_USTRUCT_BODY()

	// Query input
	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	TWeakObjectPtr<class AActor> Actor;

	UPROPERTY()
	TWeakObjectPtr<class UPrimitiveComponent> CollisionComponent;

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	FVector Origin;

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	FVector Destination;	

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	FDoNNavigationQueryParams QueryParams;

	FDoNNavigationDebugParams DebugParams;

	// Unbound:
	FVector OriginVolumeCenter;
	FVector DestinationVolumeCenter;

	FDonVoxelCollisionProfile VoxelCollisionProfile;

	// Processing state variables	
	bool bGoalFound = false;
	bool bGoalOptimized = false;	
	FDonNavigationVoxel* OriginVolume;
	FDonNavigationVoxel* DestinationVolume;	

	DoNNavigation::PriorityQueue<FDonNavigationVoxel*> Frontier;	
	TMap<FDonNavigationVoxel*, uint32> VolumeVsCostMap;
	TMap<FDonNavigationVoxel*, FDonNavigationVoxel*> VolumeVsGoalTrajectoryMap;

	DoNNavigation::PriorityQueue<FLocVector> Frontier_Unbound;
	TMap<FLocVector, uint32> VolumeVsCostMap_Unbound;
	TMap<FLocVector, FLocVector> VolumeVsGoalTrajectoryMap_Unbound;

	// Optimization state variables
	bool bOptimizationInProgress = false;
	int32 optimizer_i = 0;
	int32 optimizer_j = 0;

	// Iteration Stats	
	int32 SolverIterationCount = 0;
	float SolverTimeTaken = 0.f;

	// Solution			
	TArray<FDonNavigationVoxel*> VolumeSolution;
	TArray<FDonNavigationVoxel*> VolumeSolutionOptimized;

	TArray<FVector> PathSolutionRaw;	

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	TArray<FVector> PathSolutionOptimized;

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	EDonNavigationQueryStatus QueryStatus = EDonNavigationQueryStatus::Unscheduled;
	
	
	FDoNNavigationQueryData(){}

	FDoNNavigationQueryData(AActor* Actor, UPrimitiveComponent* CollisionComponent, FVector Origin, FVector Destination, const FDoNNavigationQueryParams& QueryParams,
		                    const FDoNNavigationDebugParams& DebugParams,  FDonNavigationVoxel* OriginVolume, FDonNavigationVoxel* DestinationVolume, 
							FVector OriginVolumeCenter, FVector DestinationVolumeCenter, FDonVoxelCollisionProfile VoxelCollisionProfile)
		                  : Actor(Actor), CollisionComponent(CollisionComponent), Origin(Origin), Destination(Destination), QueryParams(QueryParams), DebugParams(DebugParams), 
		                    OriginVolume(OriginVolume), DestinationVolume(DestinationVolume), 
		                    OriginVolumeCenter(OriginVolumeCenter), DestinationVolumeCenter(DestinationVolumeCenter), VoxelCollisionProfile(VoxelCollisionProfile){}

	FORCEINLINE FString GetActorName() { return Actor.IsValid() ? Actor->GetName() : FString();	}

	void BeginOptimizationCycle()
	{
		optimizer_i = 0;
		optimizer_j = PathSolutionRaw.Num() - 1;

		if (PathSolutionRaw.Num())
		{
			PathSolutionOptimized.Add(PathSolutionRaw[0]);
			VolumeSolutionOptimized.Reserve(PathSolutionRaw.Num());			
		}		

		bOptimizationInProgress = true;
	}

	FORCEINLINE bool MaxSweepAttemptsReachedForNode()
	{
		return PathSolutionRaw.Num() - optimizer_j > QueryParams.MaxOptimizerSweepAttemptsPerNode;
	}

};

// Non-Dynamic verison of result delegate: 
//
// DECLARE_DELEGATE_OneParam( FDoNNavigationResultHandler, const FDoNNavigationQueryData& )
//
// Consider enabling this for C++ users depending on performance implications of the currently used dynamic delegates (Epic documentation states dynamic delegates are slower).
// This will require maintenance of two sets of delegate types across the board and a separate API entry point for C++/BP users (UFUNCTIONs only accept dynamic delegate parameters).
// @todo: profile this with high volume load once the system is stable to verify if there are any notable performance benefits in using static delegates.

DECLARE_DYNAMIC_DELEGATE_OneParam(FDoNNavigationResultHandler, const FDoNNavigationQueryData&, Data);

USTRUCT()
struct FDonNavigationTask
{
	GENERATED_USTRUCT_BODY()

	virtual void BroadcastResult() {}
};

USTRUCT()
struct FDonNavigationQueryTask : public FDonNavigationTask
{
	GENERATED_USTRUCT_BODY()	

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	FDoNNavigationQueryData Data;

	UPROPERTY()
	FDoNNavigationResultHandler ResultHandler;

	UPROPERTY()
	FDonNavigationDynamicCollisionDelegate DynamicCollisionListener;

	FDonNavigationQueryTask() {}

	FDonNavigationQueryTask( FDoNNavigationQueryData InData, FDoNNavigationResultHandler ResultHandler, FDonNavigationDynamicCollisionDelegate DynamicCollisionNotifier)
						   : Data(InData), ResultHandler(ResultHandler), DynamicCollisionListener(DynamicCollisionNotifier)
	{	
		if (!InData.OriginVolume) // Unbound
		{
			Data.Frontier_Unbound.put(InData.OriginVolumeCenter, 0);
			Data.VolumeVsCostMap_Unbound.Add(InData.OriginVolumeCenter, 0);
		}
		else
		{
			Data.Frontier.put(InData.OriginVolume, 0);
			Data.VolumeVsCostMap.Add(InData.OriginVolume, 0);
		}
		
		Data.QueryStatus = EDonNavigationQueryStatus::InProgress;
	}

	FORCEINLINE bool IsQueryComplete()
	{
		return Data.QueryStatus != EDonNavigationQueryStatus::InProgress;
	}

	virtual void BroadcastResult() override
	{
		ResultHandler.ExecuteIfBound(Data);
	}
};

//DECLARE_DYNAMIC_DELEGATE(FDonCollisionSamplerCallback);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDonCollisionSamplerCallback, bool, bTaskSuccessful);

struct FDonMeshIdentifier
{	
	TWeakObjectPtr<class UPrimitiveComponent> Mesh;
	
	FName CustomCacheIdentifier;

	FName UniqueTag;

	uint32 HashValue;

	friend bool operator== (const FDonMeshIdentifier& A, const FDonMeshIdentifier& B)
	{
		return A.UniqueTag.IsEqual(B.UniqueTag);			
	}

	friend uint32 GetTypeHash(const FDonMeshIdentifier& Other)
	{	
		return Other.HashValue;
	}	

	FDonMeshIdentifier(){}

	explicit FDonMeshIdentifier(UPrimitiveComponent* Mesh, FName CustomCacheIdentifier = FName()) : Mesh(Mesh), CustomCacheIdentifier(CustomCacheIdentifier)
	{
		UniqueTag = CustomCacheIdentifier.IsNone() ? GetUniqueMeshTag(Mesh) : CustomCacheIdentifier;

		if (CustomCacheIdentifier.IsNone() && Mesh)
			HashValue = GetTypeHash(Mesh);
		else
			HashValue = GetTypeHash(CustomCacheIdentifier);
	}

private:

	FName GetUniqueMeshTag(UPrimitiveComponent* Mesh)
	{	
		FString meshName = Mesh->GetName();		
		FString ownerName = Mesh->GetOwner()->GetName();
		FString delimiter = FString("-");
		
		if (Mesh)
			return FName(*meshName.Append(delimiter).Append(ownerName));
		else
			return NAME_None;
	}
};

USTRUCT()
struct FDonNavigationDynamicCollisionTask : public FDonNavigationTask
{
	GENERATED_USTRUCT_BODY()

	FDonMeshIdentifier MeshId;
	uint32 TaskHashValue;

	FDonCollisionSamplerCallback ResultHandler;
	
	FDonNavigationVoxel MeshOriginalVolume;

	FVector MeshOriginalExtents;

	FString MeshAssetName;

	bool bReloadCollisionCache = false;

	bool bDisableCacheUsage = false;	

	bool bUseCheapBoundsCollision = false;
	
	float BoundsScaleFactor = 1.f;
	
	FCollisionObjectQueryParams ObjectParams;
	FCollisionQueryParams CollisionParams;

	bool bDrawDebug = false;

	// Internal processing:
	int32 i, j, k;
	int xLength, yLength, zLength;	
	bool bCollisionProfileSamplingComplete = false;
	bool bCollisionFetchSuccess = false;
	bool bCollisionOccupancyUpdatesComplete = false;

	float TimeTaken = 0.f;	

	// Results:
	FDonVoxelCollisionProfile CollisionData;

	void FetchSuccess()
	{
		bCollisionProfileSamplingComplete = true;
		bCollisionFetchSuccess = true;
	}

	void FetchFailure()
	{
		bCollisionProfileSamplingComplete = true;
		bCollisionFetchSuccess = false;
	}

	virtual void BroadcastResult() override
	{
		ResultHandler.ExecuteIfBound(bCollisionFetchSuccess);
	}

	FDonNavigationDynamicCollisionTask(){}

	FDonNavigationDynamicCollisionTask(FDonMeshIdentifier MeshId, FDonCollisionSamplerCallback ResultHandler, FDonNavigationVoxel MeshOriginalVolume, bool bDisableCacheUsage, bool bReloadCollisionCache, bool bUseCheapBoundsCollision, float BoundsScaleFactor, bool bDrawDebug)
		: MeshId(MeshId), ResultHandler(ResultHandler), MeshOriginalVolume(MeshOriginalVolume), bDisableCacheUsage(bDisableCacheUsage), bReloadCollisionCache(bReloadCollisionCache), bUseCheapBoundsCollision(bUseCheapBoundsCollision), BoundsScaleFactor(BoundsScaleFactor), bDrawDebug(bDrawDebug)
	{
		i = j = k = 0;

		TaskHashValue = GetTypeHash(MeshId.Mesh);
	}

	friend bool operator== (const FDonNavigationDynamicCollisionTask& A, const FDonNavigationDynamicCollisionTask& B)
	{
		return A.MeshId.Mesh == B.MeshId.Mesh;
	}
	
	friend uint32 GetTypeHash(const FDonNavigationDynamicCollisionTask& Other)	
	{	
		return Other.TaskHashValue; //GetTypeHash(Other.MeshId);
	}
};

/**
 * 
 */
UCLASS()
class DONAINAVIGATION_API ADonNavigationManager : public AActor
{
	GENERATED_BODY()

public:
	ADonNavigationManager(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

protected:

	FCollisionShape VoxelCollisionShape;
	FCollisionObjectQueryParams VoxelCollisionObjectParams;
	FCollisionQueryParams VoxelCollisionQueryParams;
	FCollisionQueryParams VoxelCollisionQueryParams2;
	TMap<FDonNavigationVoxel*, TArray <FDonNavigationVoxel*>> NavGraphCache;
	TMap<FDonMeshIdentifier, FDonVoxelCollisionProfile> VoxelCollisionProfileCache;

	bool bRegistrationCompleteForComponents;
	int32 RegistrationIndexCurrent;
	int32 MaxRegistrationsPerTick;	

public:

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	bool bIsUnbound = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Translation)
	USceneComponent* SceneComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Translation)
	UBillboardComponent* Billboard;	

	UPROPERTY(BlueprintReadOnly, Category = "DoN Navigation")
	FDonNavVoxelXYZ NAVVolumeData;

	/* Represents the side of the cube used to build the voxel. Eg: a value of 300 produces a cube 300x300x300*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Dimensions")
	float VoxelSize;

	/* The number of voxels to build along the X axis (offset from NAV actor)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Dimensions")
	int32 XGridSize;

	/* The number of voxels to build along the Y axis (offset from NAV actor)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Dimensions")
	int32 YGridSize;

	/* The number of voxels to build along the Z axis (offset from NAV actor)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Dimensions")
	int32 ZGridSize;

	// Collision

	/* Any channels added here will be treated as obstacles by the path finder*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObstacleDetection)
	TArray<TEnumAsByte<ECollisionChannel>> ObstacleQueryChannels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObstacleDetection)
	TArray<AActor*> ActorsToIgnoreForCollision;

	/* Some pathfinding scenarios need a special auto-correction to be applied to either origin or 
	* destination for pathfinding to work. Eg: If a player is hiding flush with a wall then the pathfinding origin
	* must be offset slightly adjacent to the wall.
	* Different games/maps will need different auto-correction values, some maps may need large correction values
	* while others may need only small adjustments. Tweak this list based on your game's needs.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObstacleDetection)
	TArray<float> AutoCorrectionGuessList;

	/** If set to true, collision checks will be performed for each and every voxel when the game begins. Warning: This can slow down loading of the game significantly.
	 *  Default behavior is set to false, meaning collision data will always be lazy loaded upn demand. This is the recommended approach */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Startup")
	bool PerformCollisionChecksOnStartup;

	// Performance setings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
	int32 MaxPathSolverIterationsPerTick = 500;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
	int32 MaxCollisionSolverIterationsPerTick = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings - Multithreading")
	bool bMultiThreadingEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings - Multithreading")
	int32 MaxPathSolverIterationsOnThread = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings - Multithreading")
	int32 MaxCollisionSolverIterationsOnThread = 500;

	// World generation
	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void ConstructBuilder();

	// Debug helpers:

	UPROPERTY()
	UBoxComponent* WorldBoundaryVisualizer;

	/* This property will help you identify issues with your dynamic collision setup by performinge extra vaildations at run-time.
	This can be expensive so it is disabled by default. Enable if, for example, your pawns are reacting to dynamic collisions that they shouldn't actually be interested in.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDisplayWorldBoundary = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDisplayWorldBoundaryInGame = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float DebugVoxelsLineThickness = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bRunDebugValidationsForDynamicCollisions = false;

	//

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void Debug_ToggleWorldBoundaryInGame();
	
	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void Debug_DrawAllVolumes(float LineThickness);

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void Debug_DrawVolumesAroundPoint(FVector Location, int32 CubeSize, bool DrawPersistentLines, float Duration, float LineThickness, bool bAutoInitializeVolumes = false);

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void Debug_DrawVoxelCollisionProfile(UPrimitiveComponent* MeshOrPrimitive, bool bDrawPersistent = false, float Duration = 2.f);

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")	
	void Debug_ClearAllVolumes();

	void Debug_RecalculateWorldBounds()
	{
		WorldBoundaryVisualizer->SetRelativeLocation(WorldBoundsExtent());
		WorldBoundaryVisualizer->SetBoxExtent(WorldBoundsExtent());
	}

private:
	
	// Graph generation
	void GenerateNavigationVolumePixels();	
	void BuildNAVNetwork();
	void DiscoverNeighborsForVolume(int32 x, int32 y, int32 z, TArray<FDonNavigationVoxel*>& neighbors);
	void AppendImplictDOFNeighborsForVolume(int32 x, int32 y, int32 z, TArray<FDonNavigationVoxel*>& Neighbors);
	TArray<FDonNavigationVoxel*> FindOrSetupNeighborsForVolume(FDonNavigationVoxel* Volume);

protected:

	float VoxelSizeSquared;

	// DOF based travel
	static const int32 Volume6DOF = 6;	       // 6 degrees of freedm: Forward, Backward, Left, Right, Up and Down
	static const int32 VolumeImplicitDOF = 12; // Implicit degrees of freedom: formed by the combination of any 2 direct degrees of freedom proving implicit access to a diagonal neighboring voxel

	// 6 DOF - directly usable for travel
	//						        0    1    2     3    4    5   
	int x6DOFCoords[Volume6DOF] = { 0,   0,   1,   -1,   0,   0  };
	int y6DOFCoords[Volume6DOF] = { 1,  -1,   0,    0,   0,   0, };
	int z6DOFCoords[Volume6DOF] = { 0,   0,   0,    0,   1,  -1, };	

	// Note:- 26 DOF cannot be directly used for travel as access to (x, y, z) does not guarantee access to (x + 1, y, z + 1) (etc)
	// unless both (x + 1, y, z) and (x, y, z + 1) are also accessible). In the latter case, such DOFs are referred to as implicit DOFs. 12 such implict DOFs are currently used.

	// 26 DOF table (for reference only)
	//	   0       1        2        3       4        5        6       7         8       9      10        11     12      13       14      15      16      17      18     19      20     21      22    23        24      25     26
	// { x + 1,    x,     x - 1,   x + 1,    x,     x - 1,   x + 1,	  x,    x - 1,   x + 1,	  x,	 x - 1,  x + 1, /* x,*/  x - 1,  x + 1,   x,    x - 1,  x + 1,    x,    x - 1,  x + 1,   x,   x - 1,   x + 1,   x,	   x - 1 };
	// { y - 1,  y - 1,   y - 1,     y,      y,        y,    y + 1,	y + 1,  y + 1,	 y - 1,  y - 1,	 y - 1,	   y,   /* y,*/   y,     y + 1, y + 1,  y + 1,  y - 1,  y - 1,  y - 1,    y,     y,     y,     y + 1,  y + 1,  y + 1 };
	// { z + 1,  z + 1,   z + 1,   z + 1,   z + 1,  z + 1,   z + 1,	z + 1,  z + 1,	   z,	  z,	   z,      z,   /* z,*/   z,       z,     z,      z,	z - 1,  z - 1,  z - 1,  z - 1,  z - 1, z - 1,  z - 1,  z - 1,  z - 1, };		
	

private:
	
	// Scheduled Tasks:
	TArray<FDonNavigationQueryTask,	TInlineAllocator<25>>  ActiveNavigationTasks;
	TSet<FDonNavigationDynamicCollisionTask>               ActiveDynamicCollisionTasks;

	// Multi-threading
	friend class FDonNavigationWorker;
	class FDonNavigationWorker* WorkerThread;

	TQueue<FDonNavigationQueryTask>			   CompletedNavigationTasks;
	TQueue<FDonNavigationDynamicCollisionTask> CompletedCollisionTasks;
	TQueue<FDonNavigationVoxel*> DynamicCollisionBroadcastQueue;

	void ReceiveAsyncResults();
	void ReceiveAsyncDynamicCollisionUpdates();
	void DrawAsyncDebugRequests();

	// Multi-threading - draw debug
	TQueue<FDrawDebugLineRequest>   DrawDebugLinesQueue;
	TQueue<FDrawDebugPointRequest>  DrawDebugPointsQueue;
	TQueue<FDrawDebugVoxelRequest>  DrawDebugVoxelsQueue;
	TQueue<FDrawDebugSphereRequest> DrawDebugSpheresQueue;

	void DrawDebugLine_Safe(UWorld* World, FVector LineStart, FVector LineEnd, FColor Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness);
	void DrawDebugPoint_Safe(UWorld* World, FVector PointLocation, float PointThickness, FColor Color, bool bPersistentLines, float LifeTime);
	void DrawDebugSphere_Safe(UWorld* World, FVector Center, float Radius, float Segments, FColor Color, bool bPersistentLines, float LifeTime);
	void DrawDebugVoxel_Safe(UWorld* World, FVector Center, FVector Box, FColor Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness);

	// Logging:
	void InvalidVolumeErrorLog(FDonNavigationVoxel* OriginVolume, FDonNavigationVoxel* DestinationDestination, FVector Origin, FVector Destination);
	TSet<FVector> UnresolvableVectors;


public:

	// Utility functions
	
	FORCEINLINE FVector NavVolumeExtent() { return FVector(VoxelSize / 2, VoxelSize / 2, VoxelSize / 2); }

	FORCEINLINE FVector WorldBoundsExtent() { return FVector(XGridSize * VoxelSize / 2, YGridSize * VoxelSize / 2, ZGridSize * VoxelSize / 2); }	

	FORCEINLINE FString VoxelUniqueKey(int x, int y, int z) { return FString::Printf(TEXT("%d-%d-%d"), x, y, z); }

	FORCEINLINE bool IsValidVolume(int x, int y, int z)
	{
		return NAVVolumeData.X.IsValidIndex(x) && NAVVolumeData.X[x].Y.IsValidIndex(y) && NAVVolumeData.X[x].Y[y].Z.IsValidIndex(z);
	}

	inline FVector LocationAtId(int32 X, int32 Y, int32 Z)
	{
		return GetActorLocation() + VoxelSize * FVector(X, Y, Z) + FVector(VoxelSize / 2, VoxelSize / 2, VoxelSize / 2);
	}

	inline FVector LocationAtId(FVector Location, int32 X, int32 Y, int32 Z)
	{
		return Location + VoxelSize * FVector(X, Y, Z);
	}

	inline FVector VolumeIdAt(FVector WorldLocation)
	{
		int32 x = ((WorldLocation.X - GetActorLocation().X) / VoxelSize) + (WorldLocation.X < GetActorLocation().X ? -1 : 0);
		int32 y = ((WorldLocation.Y - GetActorLocation().Y) / VoxelSize) + (WorldLocation.Y < GetActorLocation().Y ? -1 : 0);
		int32 z = ((WorldLocation.Z - GetActorLocation().Z) / VoxelSize) + (WorldLocation.Z < GetActorLocation().Z ? -1 : 0);

		return FVector(x, y, z);
	}

	inline FVector VolumeOriginAt(FVector WorldLocation)
	{
		FVector volumeId = VolumeIdAt(WorldLocation);

		return LocationAtId(volumeId.X, volumeId.Y, volumeId.Z);

	}

	inline FDonNavigationVoxel* VolumeAt(FVector WorldLocation)
	{
		int32 x = (WorldLocation.X - GetActorLocation().X) / VoxelSize;
		int32 y = (WorldLocation.Y - GetActorLocation().Y) / VoxelSize;
		int32 z = (WorldLocation.Z - GetActorLocation().Z) / VoxelSize;

		if (IsValidVolume(x, y, z))
			return &NAVVolumeData.X[x].Y[y].Z[z];
		else
			return NULL;
	}	

	inline FDonNavigationVoxel* VolumeAtSafe(int32 x, int32 y, int32 z)
	{
		if (IsValidVolume(x, y, z))
			return &NAVVolumeData.X[x].Y[y].Z[z];
		else
			return NULL;
	}

	/* Fetch volume by index. Unsafe, because it assumes that you've already checked index validity. It is faster, but will crash with invalid input
	   i.e. similar to Unreal's GetUnsafeNormal() function */
	inline FDonNavigationVoxel& VolumeAtUnsafe(int32 x, int32 y, int32 z)
	{
		return NAVVolumeData.X[x].Y[y].Z[z];
	}

	inline FDonNavigationVoxel* NeighborAt(FDonNavigationVoxel* Volume, FVector NeighborOffset)
	{
		if (!Volume)
			return NULL;

		int32 x = Volume->X + NeighborOffset.X;
		int32 y = Volume->Y + NeighborOffset.Y;
		int32 z = Volume->Z + NeighborOffset.Z;

		return VolumeAtSafe(x, y, z);
	}

	/* Clamps a vector to the navigation bounds as defined by the grid configuraiton of the navigation object you've placed in the map*/
	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	FVector ClampLocationToNavigableWorld(FVector DesiredLocation)
	{
		if (bIsUnbound)
			return DesiredLocation;

		FVector origin = GetActorLocation();
		float xClamped = FMath::Clamp(DesiredLocation.X, origin.X, origin.X + XGridSize * VoxelSize);
		float yClamped = FMath::Clamp(DesiredLocation.Y, origin.Y, origin.Y + YGridSize * VoxelSize);
		float zClamped = FMath::Clamp(DesiredLocation.Z, origin.Z, origin.Z + ZGridSize * VoxelSize);

		return FVector(xClamped, yClamped, zClamped);
	}

	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	bool IsLocationWithinNavigableWorld(FVector DesiredLocation)
	{
		if (bIsUnbound)
			return true;

		FVector origin = GetActorLocation();

		return  DesiredLocation.X >= origin.X && DesiredLocation.X <= (origin.X + XGridSize * VoxelSize) &&
				DesiredLocation.Y >= origin.Y && DesiredLocation.Y <= (origin.Y + YGridSize * VoxelSize) &&
				DesiredLocation.Z >= origin.Z && DesiredLocation.Z <= (origin.Z + ZGridSize * VoxelSize);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	
	// Core API and public utility functions

public:
	
	// Pathfinder:

	/**
	*  Schedule Pathfinding Task
	*
	*  Schedules a pathfinding task to generate the shortest available path between a given actor and a desired destination.
	*  A pathfinding task can run across multiple ticks and when the results are ready the caller is immediately notified through a result handler (FDoNNavigationResultHandler)
	*  The caller must bind the result handler in advance to be notified when results are ready.
	*  If you're interested in listening to dynamic collisions that may have invalidated the path (for course correction, etc) then you must also bind the dynamic listener FDonNavigationDynamicCollisionDelegate
	*
	*  @param  Actor                    Actor which needs to navigate from one point to another. Typically (but not necessarily) a pawn. The location of this actor is treated as origin for pathfinding
	*  @param  Destination              Point in the world to which the actor needs to travel
	*  @param  QueryParams              Additional params for customizing the path finding query
	*  @param  DebugParams              Use these debug params to enable visualization of the raw and optimized paths and other debug related activities
	*  @param  ResultHandlerDelegate    You must bind a function of your choice to this delegate to be notified when pathfinding results are available for you to use
	*  @param DynamicCollisionListener  This listener allows you to be notified whenever your path solution has been invalidated by dynamic obstacles that have occupied parts of your path solution
	*									that may previously have been navigable. Typically this means you should immediately reschedule your query to obtain a revised path solution
	*/

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")		
	bool SchedulePathfindingTask(AActor* Actor, FVector Destination, UPARAM(ref) const FDoNNavigationQueryParams& QueryParams, UPARAM(ref) const FDoNNavigationDebugParams& DebugParams, FDoNNavigationResultHandler ResultHandlerDelegate, FDonNavigationDynamicCollisionDelegate DynamicCollisionListener);

	/** Aborts an existing pathfinding task for a given Actor */
	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void AbortPathfindingTask(AActor* Actor);

	/** Does this actor have an active pathfinding task already scheduled with the navigation manager? */
	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	bool HasTask(AActor* Actor) { return ActiveNavigationTasks.ContainsByPredicate([&Actor](const FDonNavigationQueryTask& task) {return task.Data.Actor.Get() == Actor; }); };


	/**
	*  Schedule Dynamic Collision Update For Mesh
	*
	*  Updates the collision status of all voxels within a mesh's visibility bounds. You should call this function whenever a dynamic obstacle in your scene 
	*  moves, scales or morphs in any way that affects the collision. Always remember that the DoN Navigation system does not automatically detect dynamic voxel collision
	*  as that would be too expensive for the system to manage (potentially millions of voxels reside in a scene). Therefore it relies on users of the plugin to manually
	*  trigger dynamic collision depending on the unique needs of a particular project. You only need this for any object that moves after the game has begun, voxel collision
	*  for static objects is exempt from this as they're mananged through a different code path and lazy-loaded on demand.
	*
	*  This function is expensive as it samples per-voxel collision so use it with care.
	*
	*  @param  Mesh								  The mesh (usually a movable object) whose collision influence needs to be updated around the current location of the mesh
	*
	*  @param  ResultHandler                      Use this delegate to be notified when the task is complete and to learn its final status
	*
	*  @param  CustomCacheIdentifier              Use this to share a single collision profile across multiple meshes or to fully customize what gets loaded into the collision cache.
	*
	*                                             Details: By default the collision cache uses the address of each mesh component as the cache key. This has two fundamental limitations:
	*											  1. Multiple meshes with identical collision properties will end up creating individual entries in the cache despite being the same (collision wise).
	*											  2. Any mesh that needs to change its rotation or scale cannot use the default cache value which only works for location based translations.
	*
	*                                             The first limitation can be easily sovled by sharing a single cache identifier across all meshes of the same type (Eg: "SolidWall_NoRotation").
	*                                             The second limitation can be resolved either by forcibly reloading the collision cache each time using bReloadCollisionCache (not recommended, very expensive) 
	*                                             OR you can use the Custom Cache Identifier to define cardinal translation points for your mesh as follows:	
	*                                             Eg: Mesh Scale = 1.5f -> CustomCacheIdentifier = "SolidWall_NoRotation_150p", 
	*                                             Eg: Mesh Scale = 2.0f -> CustomCacheIdentifier = "SolidWall_NoRotation_200p", etc. 
	*                                             Using this technique instead of forcibly reloading the cache will improve your performance. Try to keep your identifiers short though!
	*
	*  @param  bReplaceExistingTask               By default the scheduler will ignore new tasks for a mesh if it already has one running. Use this to forcibly repalce an existing task.
	*                                             This is useful for advanced usecases where you're relying on the sequence of dynamic collision updates or triggering other events dependent on its success
	*
	*  @param  bReloadCollisionCache			  Default is false. The system maintains a cache of voxel collision profiles associated with a mesh. A cache entry is created for a mesh
	*                                             the first time you call this function for it. If you really want to overwrite the cache value associated with a mesh
	*                                             then set bReloadCollisionCache to true. However for most usecases it is recommended to use CustomCacheIdentifier instead.
	*
	*
	*  @param  bUseCheapBoundsCollision			  Marks all voxels within the mesh's visibility bounds as collided. This is several times faster that per-voxel sampling. Great for simple meshes like walls
	*											  whose visibility bounds closely aligns with the actual mesh itself.
	*                                             
	*  @param  BoundsScaleFactor				  Use this multiplier to increase or decrease the size of the bounding box derived from your mesh within which voxel collisions are sampled.
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	bool ScheduleDynamicCollisionUpdate(UPrimitiveComponent* Mesh, FDonCollisionSamplerCallback ResultHandler, FName CustomCacheIdentifier = NAME_None, bool bReplaceExistingTask = false, bool bDisableCacheUsage = false, bool bReloadCollisionCache = false, bool bUseCheapBoundsCollision = false, float BoundsScaleFactor = 1.f, bool bForceSynchronousExecution = false, bool bDrawDebug = false);

	/** 
	*  Unregisters a given dynamic collision listener from a given volume. Your should always call this function whenever a particular actor or object is
	*  no longer interested in listening to collisions in a particular area. This is especially important for maintaining performance as 
	*  accumulating unwanted collision listeners will clog up the system quickly and affect performance.	
	*/
	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void StopListeningToDynamicCollisionsForPath(FDonNavigationDynamicCollisionDelegate ListenerToClear, UPARAM(ref) const FDoNNavigationQueryData& QueryData);
	
	void VoxelCacheClearByKey(const FDonMeshIdentifier &MeshId)
	{
		VoxelCollisionProfileCache.Remove(MeshId);
	}

	/** 
	*  WARNING: This function is for stress-testing for performance only, it operates synchronously unlike the scheduler functions making it great for profiling sessions.
	*  Use SchedulePathfindingTask for regular navigation and pathfinding usecases.
	*
	*  Given an actor (representing origin) and a destination point in the world, this function generates the shortest path between the two. 
	*
	*  @param  Actor                  Actor which needs to navigate from one point to another. Typically (but not necessarily) a pawn. The location of this actor is treated as origin for pathfinding	
	*  @param  Destination            Point in the world to which the actor needs to travel
	*  @param  PathSolutionRaw        Path solution as an array of FVectors representing the shortest path from origin to destination
	*  @param  PathSolutionOptimized  Path solution optimized from the raw path by performing collision sweeps using the actor's collision component
	*  @param  QueryParams            Additional params for the path finding query									  
	*  @param  DebugParams            Use these debug params to enable visualization of the raw and optimized paths and other debug related activities
	*/

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	bool FindPathSolution_StressTesting(AActor* Actor, FVector Destination, TArray<FVector> &PathSolutionRaw, TArray<FVector> &PathSolutionOptimized, UPARAM(ref) const FDoNNavigationQueryParams& QueryParams, UPARAM(ref) const FDoNNavigationDebugParams& DebugParams);	

	// Tracing utility

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	bool IsDirectPathSweep(UPrimitiveComponent* CollisionComponent, FVector Start, FVector End, FHitResult &OutHit, bool bFindInitialOverlaps = false, float CollisionShapeInflation = 0.f);	

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	bool IsDirectPathLineTrace(FVector start, FVector end, FHitResult &OutHit, const TArray<AActor*> &ActorsToIgnore, bool bFindInitialOverlaps = true);

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	bool IsDirectPathLineSweep(UPrimitiveComponent* CollisionComponent, FVector Start, FVector End, FHitResult &OutHit, bool bFindInitialOverlaps = false, float CollisionShapeInflation = 0.f);

	// Shape based:	
	bool IsDirectPathSweepShape(const FCollisionShape& Shape, FVector Start, FVector End, FHitResult &OutHit, bool bFindInitialOverlaps = false);	
	bool IsDirectPathLineSweepShape(const FCollisionShape& Shape, FVector Start, FVector End, FHitResult &OutHit, bool bFindInitialOverlaps = false);


	// AI Utility Functions
	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	FVector FindRandomPointFromActorInNavWorld(AActor* Actor, float Distance, bool& bFoundValidResult, float MaxDesiredAltitude = -1.f, float MaxZAngularDispacement = 15.f, int32 MaxAttempts = 5);

	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	FVector FindRandomPointAroundOriginInNavWorld(AActor* NavigationActor, FVector Origin, float Distance, bool& bFoundValidResult, float MaxDesiredAltitude = -1.f, float MaxZAngularDispacement = 15.f, int32 MaxAttempts = 5);

	/* This is an edge case where the goal is beneath the landscape (and therefore can never be reached). This situation should be identified pre-emptively and dealt with to prevent a futile and expensive call*/
	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	bool IsLocationBeneathLandscape(FVector Location, float LineTraceHeight = 3000.f);

	UFUNCTION(BlueprintPure, Category = "DoN Navigation")
	bool IsMeshBoundsWithinNavigableWorld(UPrimitiveComponent* Mesh, float BoundsScaleFactor = 1.f);

	// Debug helpers:
	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void VisualizeNAVResult(UPARAM(ref) const TArray<FVector>& PathSolution, FVector Source, FVector Destination, bool Reset, UPARAM(ref) const FDoNNavigationDebugParams& DebugParams, UPARAM(ref) FColor const& LineColor);

	UFUNCTION(BlueprintCallable, Category = "DoN Navigation")
	void VisualizeDynamicCollisionListeners(FDonNavigationDynamicCollisionDelegate Listener, UPARAM(ref) const FDoNNavigationQueryData& QueryData);

	// NAV Visualizer
	void VisualizeSolution(FVector source, FVector destination, const TArray<FVector>& PathSolutionRaw, const TArray<FVector>& PathSolutionOptimized, const FDoNNavigationDebugParams& DebugParams);

	// Logging utility
	static FString GetMeshLogIdentifier(UPrimitiveComponent* Mesh);
	
	////////////////////////////////////////////////////////////////////////////////////////

private:

	// Core pathfinding algorithms
	void TickScheduledPathfindingTasks(float DeltaSeconds, int32 MaxIterationsPerTick);	
	void TickScheduledPathfindingTasks_Safe(float DeltaSeconds, int32 MaxIterationsPerTick);
	void TickScheduledCollisionTasks(float DeltaSeconds, int32 MaxIterationsPerTick);	
	void TickScheduledCollisionTasks_Safe(float DeltaSeconds, int32 MaxIterationsPerTick);

protected:
	virtual void TickNavigationSolver(FDonNavigationQueryTask& task);
	virtual bool PrepareSolution(FDonNavigationQueryTask& Task);

private:
	void TickNavigationOptimizer(FDonNavigationQueryTask& task);
	void TickNavigationOptimizerCycle(FDonNavigationQueryTask& task, int32& IterationsProcessed, const int32 MaxIterationsPerTask);
	void TickVoxelCollisionSampler(FDonNavigationDynamicCollisionTask& Task);
	void ExpandFrontierTowardsTarget(FDonNavigationQueryTask& Task, FDonNavigationVoxel* Current, FDonNavigationVoxel* Neighbor);
	void PackageRawSolution(FDonNavigationQueryTask& task);

	// Thread-aware routines
	FCriticalSection CriticalSection_Pathing;
	FCriticalSection CriticalSection_Collisions;
	void AddPathfindingTask(FDonNavigationQueryTask& Task);
	void AddDynamicCollisionTask(FDonNavigationDynamicCollisionTask& Task);
	bool IsDynamicCollisionTaskActive(FDonNavigationDynamicCollisionTask& Task);
	void CompleteNavigationTask(int32 TaskIndex);
	void CompleteCollisionTask(FDonNavigationDynamicCollisionTask& Task, bool bIsSuccess);

	void AbortPathfindingTask_Internal(AActor* Actor);
	void AbortPathfindingTaskByIndex(int32 TaskIndex);	
	inline void CleanupExistingTaskForActor(AActor* Actor) { AbortPathfindingTask(Actor); }

	// Voxel collision sampling:
	void UpdateVoxelCollision(FDonNavigationVoxel& Volume);
	FDonVoxelCollisionProfile GetVoxelCollisionProfileFromMesh(const FDonMeshIdentifier& MeshId, bool &bResultIsValid, bool bIgnoreMeshOriginOccupancy = false, bool bDisableCacheUsage = false, FName CustomCacheIdentifier = NAME_None, bool bReloadCollisionCache = false, bool bUseCheapBoundsCollision = false, float BoundsScaleFactor = 1.f, bool DrawDebug = false);
	FDonVoxelCollisionProfile SampleVoxelCollisionForMesh(UPrimitiveComponent* Mesh, bool &bResultIsValid, bool bIgnoreMeshOriginOccupancy = false, FName CustomCacheIdentifier = NAME_None, bool bUseCheapBoundsCollision = false, float BoundsScaleFactor = 1.f, bool DrawDebug = false);

	// Dynamic collision listeners:
	void DynamicCollisionUpdateForMesh(const FDonMeshIdentifier& MeshId, FDonVoxelCollisionProfile& VoxelCollisionProfile, bool bDisableCacheUsage = false, bool bDrawDebug = false);
	void AddCollisionListenerToVolumeFromTask(FDonNavigationVoxel* Volume, FDonNavigationQueryTask& task);
	FDonNavigationVoxel* AppendVolumeList(FVector Location, FDonNavigationQueryTask& task);
	void AppendVolumeListFromRange(FVector Start, FVector End, FDonNavigationQueryTask& task);

	FDonNavigationVoxel* ResolveVector(FVector &DesiredLocation, UPrimitiveComponent* CollisionComponent, bool bFlexibleOriginGoal = true, float CollisionShapeInflation = 0.f, bool bShouldSweep = true);	
	FDonNavigationVoxel* GetClosestNavigableVolume(FVector DesiredLocation, UPrimitiveComponent* CollisionComponent, bool &bInitialPositionCollides, float CollisionShapeInflation = 0.f, bool bShouldSweep = true);	
	FDonNavigationVoxel* GetBestNeighborRecursive(FDonNavigationVoxel* Volume, int32 CurrentDepth, int32 NeighborSearchMaxDepth, FVector Location, UPrimitiveComponent* CollisionComponent, bool bConsiderInitialOverlaps, float CollisionShapeInflation, bool bShouldSweep);

	bool ResolveVector(FVector &DesiredLocation, FVector &ResolvedLocation, UPrimitiveComponent* CollisionComponent, bool bFlexibleOriginGoal = true, float CollisionShapeInflation = 0.f, bool bShouldSweep = true);
	bool GetClosestNavigableVector(FVector DesiredLocation, FVector &ResolvedLocation, UPrimitiveComponent* CollisionComponent, bool &bInitialPositionCollides, float CollisionShapeInflation = 0.f, bool bShouldSweep = true);

protected:

	bool CanNavigate(FVector Location);
	bool CanNavigate(FDonNavigationVoxel* Volume);
	bool CanNavigateByCollisionProfile(FDonNavigationVoxel* Volume, const FDonVoxelCollisionProfile& CollisionToTest);
	bool CanNavigateByCollisionProfile(FVector Location, const FDonVoxelCollisionProfile& CollisionToTest);

private:

	// Path solution generation and optimization pass
	void PathSolutionFromVolumeSolution(const TArray<FDonNavigationVoxel*>& VolumeSolution, TArray<FVector> &PathSolution, FVector Origin, FVector Destination, const FDoNNavigationDebugParams& DebugParams);	
	void OptimizePathSolution(UPrimitiveComponent* CollisionComponent, const TArray<FVector>& PathSolution, TArray<FVector> &PathSolutionOptimized, float CollisionShapeInflation = 0.f);	
	void OptimizePathSolution_Pass1_LineTrace(UPrimitiveComponent* CollisionComponent, const TArray<FVector>& PathSolution, TArray<FVector> &PathSolutionOptimized, float CollisionShapeInflation = 0.f);

	////////////////////////////////////////////////////////////////////////////////////////
};