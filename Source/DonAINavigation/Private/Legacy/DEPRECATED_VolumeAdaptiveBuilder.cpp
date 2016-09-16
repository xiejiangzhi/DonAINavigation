// Copyright(c) 2015 Venugopalan Sreedharan. All rights reserved.

#include "DonAINavigationPrivatePCH.h"

#include "Legacy/DEPRECATED_VolumeAdaptiveBuilder.h"
#include "Kismet/KismetSystemLibrary.h"
#include "StaticMeshResources.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include <stdio.h>
#include <time.h>

uint64 NAVOverlapQueryTime = 0;
uint64 ObstacleOverlapQueryTime = 0;
uint64 navGenerationTime;
uint64 navNeighborInferenceTime;

ADEPRECATED_VolumeAdaptiveBuilder::ADEPRECATED_VolumeAdaptiveBuilder(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComp"));
	SceneComponent->Mobility = EComponentMobility::Static;
	RootComponent = SceneComponent;

	Billboard = ObjectInitializer.CreateDefaultSubobject<UBillboardComponent>(this, TEXT("Billboard"));
	//static ConstructorHelpers::FObjectFinder<UTexture2D> BillboardTexture(TEXT("Texture2D'/Game/AI/NavigationVolumes/Navigation_Volumer_Aerial.Navigation_Volumer_Aerial'"));
	//Billboard->Sprite = BillboardTexture.Object;
	Billboard->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	XBaseUnit = 150;
	YBaseUnit = 150;
	ZBaseUnit = 150;

	XGridSize = 200;
	YGridSize = 200;
	ZGridSize = 25;

	XYAspectRatioThreshold = 20;
	UseAspectRatioThreshold = true;
	
	OffsetThickness = 1;
	FloorClearance = 20;

	//#define ECC_DoNNavigationVolume	ECC_GameTraceChannel3;

	ObstacleList.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	// If you're testing you need to create a collision object of this name and uncomment:
	// NAVOverlapQuery.Add(UEngineTypes::ConvertToObjectType(COLLISION_DONNAVIGATIONVOLUME));

	GenerateNavigationVolumes = false;
	RegenerateNAVNetwork = false;
	DisplayNAVNeighborGraph = false;
	IsVisibleNavigableVolumes = true;
	CleanUpAllData = false;
}

void ADEPRECATED_VolumeAdaptiveBuilder::CleanUp()
{
	for (UDoNNavigationVolumeComponent* volume : NAVVolumeComponents)
		if (volume)
			volume->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			//volume->bCreatedByConstructionScript_DEPRECATED = true;

	DestroyConstructedComponents();
	NAVVolumeComponents.Empty();

	FlushPersistentDebugLines(GetWorld());
}

void ADEPRECATED_VolumeAdaptiveBuilder::ConstructBuilder()
{
	if (!GenerateNavigationVolumes)
		return;

	UWorld* const World = GetWorld();
	if (!World)
		return;	

	CleanUp();	

	navGenerationTime = DoNNavigation::Debug_GetTimer();
	GenerateAdaptiveNavigationVolumeSeeds();
	DoNNavigation::Debug_StopTimer(navGenerationTime);

	FString counter1 = FString::Printf(TEXT("Time spent generating %d NAV volumes: %f seconds"), NAVVolumeComponents.Num(), navGenerationTime / 1000.0);
	UE_LOG(LogTemp, Log, TEXT("%s"), *counter1);	
	
	navNeighborInferenceTime = DoNNavigation::Debug_GetTimer();
	BuildNAVNetwork();
	DoNNavigation::Debug_StopTimer(navNeighborInferenceTime);

	FString counter2 = FString::Printf(TEXT("Time spent inferring NAV neighbors: %f seconds"), navNeighborInferenceTime / 1000.0);
	UE_LOG(LogTemp, Log, TEXT("%s"), *counter2);
}

void ADEPRECATED_VolumeAdaptiveBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);	
}

#if WITH_EDITOR
void ADEPRECATED_VolumeAdaptiveBuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	FName PropertyName = PropertyThatChanged != NULL ? PropertyThatChanged->GetFName() : NAME_None;	

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADEPRECATED_VolumeAdaptiveBuilder, IsVisibleNavigableVolumes) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ADEPRECATED_VolumeAdaptiveBuilder, IsVisibleBlockedVolumes) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ADEPRECATED_VolumeAdaptiveBuilder, IsVisibleInGame)
		)
	{
		for (UDoNNavigationVolumeComponent* volume : NAVVolumeComponents)
		{
			if (!volume)
				continue;

			if (volume->CanNavigate)
				volume->SetVisibility(IsVisibleNavigableVolumes);
			else
				volume->SetVisibility(IsVisibleBlockedVolumes);

			volume->SetHiddenInGame(!IsVisibleInGame);
		}
	}

	if (GenerateNavigationVolumes)
	{
		ADEPRECATED_VolumeAdaptiveBuilder::ConstructBuilder();
		GenerateNavigationVolumes = false;
	}	
	
	if (RegenerateNAVNetwork || PropertyName == GET_MEMBER_NAME_CHECKED(ADEPRECATED_VolumeAdaptiveBuilder, DisplayNAVNeighborGraph))
	{
		FlushPersistentDebugLines(GetWorld());		
		BuildNAVNetwork();
		RegenerateNAVNetwork = false;
	}

	if (CleanUpAllData)
	{
		CleanUp();
		CleanUpAllData = false;
	}

	
}
#endif // WITH_EDITOR

UDoNNavigationVolumeComponent* ADEPRECATED_VolumeAdaptiveBuilder::CreateNAVVolume(FVector Location, FName VolumeName, int32 SeedX, int32 SeedY, int32 SeedZ)
{
	UDoNNavigationVolumeComponent* volumeComponent = NewObject<UDoNNavigationVolumeComponent>(this, UDoNNavigationVolumeComponent::StaticClass(), VolumeName);

	if (!volumeComponent)
	{
		FString counter = FString::Printf(TEXT("Could not generate NAV Volume %s at %s for seed x%d, y%d, z%d"), *VolumeName.ToString(), *Location.ToString(), SeedX, SeedY, SeedZ);		
		UE_LOG(LogTemp, Log, TEXT("%s"), *counter);

		return NULL;
	}	
	
	volumeComponent->SetWorldLocation(Location);
	volumeComponent->SetBoxExtent(FVector(0, 0, 0));
	volumeComponent->X = SeedX;
	volumeComponent->Y = SeedY;
	volumeComponent->Z = SeedZ;

	//volumeComponent->bCreatedByConstructionScript = true;

	NAVVolumeComponents.Add(volumeComponent);

	volumeComponent->RegisterComponent();	
	volumeComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	return volumeComponent;
}

void ADEPRECATED_VolumeAdaptiveBuilder::GenerateAdaptiveNavigationVolumeSeeds()
{	
	std::vector< std::vector<int> > seedTrackerZ;
	std::vector< std::vector< std::vector<bool> > > SeedTrackerXYZ;

	UWorld* const World = GetWorld();

	if (!World)
		return;
	
	UE_LOG(LogTemp, Log, TEXT("Generating adaptive volumes..."));
	
	//SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, 25));

	seedTrackerZ = std::vector< std::vector<int> >(XGridSize, std::vector<int>(YGridSize));		
	SeedTrackerXYZ = std::vector< std::vector< std::vector<bool> > >(XGridSize, std::vector< std::vector<bool> >(YGridSize, std::vector<bool>(ZGridSize)));

	for (int i = 0; i < XGridSize; i++)
	{
		for (int j = 0; j < YGridSize; j++)		
		{			
			for (int k = ZGridSize - 1 - seedTrackerZ[i][j]; k >= 0;)
			{	
				/*
				if (SeedTrackerXYZ[i][j][k])
				{
					k--;
					continue; // already inferred
				}
				*/

				// Log Progress
				//FString counter = FString::Printf(TEXT("Creating NAV Seed %d,%d,%d/%d,%d,%d"), i, j, k, XGridSize, YGridSize, ZGridSize);
				//UE_LOG(LogTemp, Log, TEXT("%s"), *counter);

				FVector Location = FVector(XBaseUnit / 2 + XBaseUnit*i + GetActorLocation().X + OffsetThickness, 
										   YBaseUnit / 2 + YBaseUnit*j + GetActorLocation().Y + OffsetThickness, 
										   ZBaseUnit / 2 + ZBaseUnit*k + GetActorLocation().Z + OffsetThickness);				

				UDoNNavigationVolumeComponent* volume = CreateNAVVolume(Location, NAME_None, i, j, k);
				if (!volume)
				{
					k--;
					continue;
				}	

				GrowNAVVolumeToCapacity(volume, seedTrackerZ, SeedTrackerXYZ );

				//k = ZGridSize - 1 - seedTrackerZ[i][j];
				k--;
			}			
		}
	}	
}

bool ADEPRECATED_VolumeAdaptiveBuilder::GrowNAVVolumeByIndex(UDoNNavigationVolumeComponent* volume, int32 XGrowth, int32 YGrowth, int32 ZGrowth)
{	
	bool infantVolume = XGrowth == 1 && ZGrowth == 1 && YGrowth == 1;

	float xBoxExtent = (XGrowth * XBaseUnit + (XGrowth - 1)*OffsetThickness) / 2;
	float yBoxExtent = (YGrowth * YBaseUnit + (YGrowth - 1)*OffsetThickness) / 2;
	float zBoxExtent = (ZGrowth * ZBaseUnit + (ZGrowth - 1)*OffsetThickness) / 2;

	float x = xBoxExtent + (XBaseUnit + OffsetThickness) * volume->X + GetActorLocation().X;
	float y = yBoxExtent + (YBaseUnit + OffsetThickness) * volume->Y + GetActorLocation().Y;	
	float z = zBoxExtent + (ZBaseUnit + OffsetThickness) * (volume->Z + 1 - ZGrowth) + GetActorLocation().Z;
	
	FVector originalWorldLocation = volume->GetComponentLocation();
	FVector originalBoxExtent = volume->GetUnscaledBoxExtent();		
	volume->SetWorldLocation(FVector(x, y, z));
	volume->SetBoxExtent(FVector(xBoxExtent, yBoxExtent, zBoxExtent));

	TArray<UPrimitiveComponent*> obstacles;	
	UKismetSystemLibrary::ComponentOverlapComponents_NEW(volume, volume->GetComponentTransform(), ObstacleList, NULL, ActorsToIgnoreForCollision, obstacles);

	if (obstacles.Num() > 0)
	{
		if (infantVolume)
		{
			/*volume->CanNavigate = false;volume->ShapeColor = FColor::Red;*/
			NAVVolumeComponents.Remove(volume);
			volume->DestroyComponent();

			return false;
		}
		else
		{
			volume->SetWorldLocation(originalWorldLocation);
			volume->SetBoxExtent(originalBoxExtent);
		}

		return false;
	}
	else
	{
		TArray<UPrimitiveComponent*> neighboringVolumeComponents;		
		UKismetSystemLibrary::ComponentOverlapComponents_NEW(volume, volume->GetComponentTransform(), NAVOverlapQuery, UDoNNavigationVolumeComponent::StaticClass(), ActorsToIgnoreForCollision, neighboringVolumeComponents);
		neighboringVolumeComponents.Remove(volume);		

		if (neighboringVolumeComponents.Num() > 0)
		{
			if (infantVolume)
			{
				// Seed has landed on existing volume, destroy it immediately
				NAVVolumeComponents.Remove(volume);
				volume->DestroyComponent();

				return false;
			}
			else
			{
				volume->SetWorldLocation(originalWorldLocation);
				volume->SetBoxExtent(originalBoxExtent);
			}

			return false;
		}
		else
		{
			volume->UpdateBounds();
			return true;
		}
			
	}
	
}

void ADEPRECATED_VolumeAdaptiveBuilder::GrowNAVVolumeToCapacity(UDoNNavigationVolumeComponent* volume, std::vector< std::vector<int> > &seedTrackerZ, std::vector< std::vector< std::vector<bool> > > &SeedTrackerXYZ)
{
	int32 XGrowth = 1;
	int32 YGrowth = 1;
	int32 ZGrowth = 1;
	int32 k = 0;	
	
	for (; k < volume->Z+1; k++)
	{
		ZGrowth = k + 1;				
		seedTrackerZ[volume->X][volume->Y]++;
		SeedTrackerXYZ[volume->X][volume->Y][volume->Z-k] = true;

		bool growthSuccessful = GrowNAVVolumeByIndex(volume, XGrowth, YGrowth, ZGrowth);
		if (!growthSuccessful)
		{
			if (k == 0)
				return;
			else
			{	
				ZGrowth = std::max(1, ZGrowth - 1);
				break;
			}
			
		}			
	}
	
	int32 j = 1, i = 1;
	bool growY = true, growX = true;

	while (growY || growX)
	{
		if (UseAspectRatioThreshold && !growX && YGrowth >= XYAspectRatioThreshold * XGrowth)
			growY = false;

		if (j >= YGridSize - volume->Y)
			growY = false;

		if (growY)
		{
			YGrowth = j + 1;
			for (int kk = 1; kk < k; kk++)
				SeedTrackerXYZ[volume->X][volume->Y + j][volume->Z - kk] = true;

			bool growthSuccessful = GrowNAVVolumeByIndex(volume, XGrowth, YGrowth, ZGrowth);
			if (!growthSuccessful)
			{
				YGrowth = std::max(1, YGrowth - 1);
				growY = false;
			}
			else
			{
				if (ZGridSize - 1 - volume->Z == seedTrackerZ[volume->X][volume->Y + j])
					seedTrackerZ[volume->X][volume->Y + j] = seedTrackerZ[volume->X][volume->Y];
				j++;
			}
		}	

		if (UseAspectRatioThreshold && !growY && XGrowth >= XYAspectRatioThreshold * YGrowth)
			growX = false;

		if (i >= XGridSize - volume->X)
			growX = false;

		if (growX)
		{
			XGrowth = i + 1;
			bool growthSuccessful = GrowNAVVolumeByIndex(volume, XGrowth, YGrowth, ZGrowth);			

			if (!growthSuccessful)
			{
				XGrowth = std::max(1, XGrowth - 1);
				growX = false;
			}
			else
			{			
				for (int kk = 1; kk < k; kk++)
					for (int jj = 1; jj < j; jj++)
					{ 
						if (ZGridSize - 1 - volume->Z == seedTrackerZ[volume->X + i][volume->Y + jj])
							seedTrackerZ[volume->X + i][volume->Y + jj] = seedTrackerZ[volume->X][volume->Y];
						SeedTrackerXYZ[volume->X+i][volume->Y + jj][volume->Z - kk] = true;
					}
					
				i++;				
			}				
		}			
	}	
}

bool VolumesOverlapAxis2(float aMin, float aMax, float bMin, float bMax)
{
	bool noOverlap = (aMin < bMin && aMax < bMin) || (aMin > bMax && aMax > bMax);
	return !noOverlap;
};

void ADEPRECATED_VolumeAdaptiveBuilder::BuildNAVNetwork()
{	
	NavGraph.nodes = TMap<UDoNNavigationVolumeComponent*, TArray<UDoNNavigationVolumeComponent*>>();
	
	for (auto Iter(NAVVolumeComponents.CreateIterator()); Iter; Iter++)
	{
		if (!Iter)
			continue;

		UDoNNavigationVolumeComponent* volume = (*Iter);		
		TArray<UPrimitiveComponent*> neighboringVolumeComponents;	

		volume->UpdateBounds(); // GetBoxExtrema functions return outdated values unless this is called after a volume has grown to desired size
		volume->CanNavigate = true;
		volume->ShapeColor = FColor::Green;		

		FVector originalBoxExtent = volume->GetUnscaledBoxExtent();		
		volume->SetBoxExtent(FVector(originalBoxExtent.X + XBaseUnit / 2, originalBoxExtent.Y + YBaseUnit / 2, originalBoxExtent.Z + ZBaseUnit/2));		
		UKismetSystemLibrary::ComponentOverlapComponents_NEW(volume, volume->GetComponentTransform(), NAVOverlapQuery, UDoNNavigationVolumeComponent::StaticClass(), ActorsToIgnoreForCollision, neighboringVolumeComponents);
		volume->SetBoxExtent(originalBoxExtent);		

		neighboringVolumeComponents.Remove(volume);
		volume->UpdateBounds();

		TArray<UDoNNavigationVolumeComponent*> neighbors;
		for (UPrimitiveComponent* neighbor : neighboringVolumeComponents)
		{
			neighbor->UpdateBounds();
			
			bool overlapsX = VolumesOverlapAxis2(volume->Bounds.GetBoxExtrema(0).X, volume->Bounds.GetBoxExtrema(1).X, neighbor->Bounds.GetBoxExtrema(0).X, neighbor->Bounds.GetBoxExtrema(1).X);
			bool overlapsY = VolumesOverlapAxis2(volume->Bounds.GetBoxExtrema(0).Y, volume->Bounds.GetBoxExtrema(1).Y, neighbor->Bounds.GetBoxExtrema(0).Y, neighbor->Bounds.GetBoxExtrema(1).Y);
			bool overlapsZ = VolumesOverlapAxis2(volume->Bounds.GetBoxExtrema(0).Z, volume->Bounds.GetBoxExtrema(1).Z, neighbor->Bounds.GetBoxExtrema(0).Z, neighbor->Bounds.GetBoxExtrema(1).Z);
			bool pathExists = (overlapsX && overlapsY) || (overlapsY && overlapsZ) || (overlapsZ && overlapsX);
			if (!pathExists)
				continue;				

			neighbors.Add(Cast<UDoNNavigationVolumeComponent>(neighbor));

			if (DisplayNAVNeighborGraph)
			{
				DrawDebugPoint(GetWorld(), volume->GetComponentLocation(), 10.f, FColor::Blue, true);
				DrawDebugLine(GetWorld(), volume->GetComponentLocation(), neighbor->GetComponentLocation(), FColor::Red, true, -1.f, 0, 4.f);
				
				/*if (volume->X == 88 && volume->Y == 169 && volume->Z == 7)
				{					
					volume->ShapeColor = FColor::Red;
					Cast<UDoNNavigationVolumeComponent>(neighbor)->ShapeColor = FColor::Blue;
				}*/

				volume->SetVisibility(true);
				neighbor->SetVisibility(true);
				
			}
		}			

		NavGraph.nodes.Add((*Iter), neighbors);

		// [Old code] Special workaround for persisting UObject volumes onto a UMAP. Not applicable for pixel builders ATM
		FNAVMapContainer NAVMapContainer;
		NAVMapContainer.volume = volume;
		NAVMapContainer.neighbors = neighbors;
		NavGraph_GCSafe.Add(NAVMapContainer);		
	}
}


void ADEPRECATED_VolumeAdaptiveBuilder::BeginPlay()
{
	Super::BeginPlay();

	UWorld* const World = GetWorld();

	if (!World)
		return;
	
	for (FNAVMapContainer NAVMapContainer : NavGraph_GCSafe)
		NavGraph.nodes.Add(NAVMapContainer.volume, NAVMapContainer.neighbors);

}

void ADEPRECATED_VolumeAdaptiveBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

// Core pathfinding algorithms begin:
bool ADEPRECATED_VolumeAdaptiveBuilder::IsDirectPath(FVector start, FVector end, FHitResult &OutHit)
{	
	TArray<AActor*> actorsToIgnore;
	TArray<TEnumAsByte<EObjectTypeQuery>> objectTypeQuery;
	objectTypeQuery.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	objectTypeQuery.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	
	bool hitSomething = UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), start, end, objectTypeQuery, false, actorsToIgnore, EDrawDebugTrace::None, OutHit, true);

	/*
	if (hitSomething)
	{
		FString priorityText = FString::Printf(TEXT("Direct path hit: %s %s"), *outHit.Actor->GetName(), *outHit.Component->GetName());
		UE_LOG(LogTemp, Log, TEXT("%s"), *priorityText);
	}
	*/	

	return !hitSomething;
}

void ADEPRECATED_VolumeAdaptiveBuilder::OptimizePathSolution_Pass1_LineTrace(const TArray<FVector>& PathSolution, TArray<FVector> &PathSolutionOptimized)
{
	if (PathSolution.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Could not perform line trace optimizations for this path - this path is empty"));
		PathSolutionOptimized = PathSolution;
		return;
	}
	
	PathSolutionOptimized.Add(PathSolution[0]);

	for (int32 i = 0; i < PathSolution.Num() - 1;)
	{
		if (i == PathSolution.Num() - 2)
		{
			PathSolutionOptimized.Add(PathSolution[PathSolution.Num() - 1]);
			return;
		}	

		bool foundDirectPath = false;

		for (int32 j = PathSolution.Num() - 1; j > i; j--)
		{
			FHitResult OutHit;
			TArray<AActor*> actorsToIgnore;

			// @todo: convert this to sweep the mesh collision shape instead. Dependent on refactoring task
			if (IsDirectPath(PathSolution[i], PathSolution[j], OutHit))
			{
				foundDirectPath = true;
				PathSolutionOptimized.Add(PathSolution[j]);
				i = j;				
				break;
			}
		}
		
		if (!foundDirectPath && i <= PathSolution.Num() - 2)
		{
			UE_LOG(LogTemp, Log, TEXT("Nav path alert: Couldn't find a single direct path among a series of points that were calculated as navigable. Potentially serious issue with pathing."));
			PathSolutionOptimized = PathSolution;
			return;
		}		
	}
}

void ADEPRECATED_VolumeAdaptiveBuilder::OptimizePathSolution(const TArray<FVector>& PathSolution, TArray<FVector> &PathSolutionOptimized)
{
	PathSolutionOptimized.Reserve(PathSolution.Num() + 2);

	OptimizePathSolution_Pass1_LineTrace(PathSolution, PathSolutionOptimized);
}

TArray<FVector> ADEPRECATED_VolumeAdaptiveBuilder::PathSolutionFromVolumeSolution(TArray<UDoNNavigationVolumeComponent*> VolumeSolution, FVector Origin, FVector Goal, bool DrawDebugVolumes)
{
	TArray<FVector> pathSolution;
	pathSolution.Add(Origin);

	for (UDoNNavigationVolumeComponent* volume : VolumeSolution)
		pathSolution.Add(volume->GetComponentLocation());

	pathSolution.Add(Goal);
	return pathSolution;


	bool VolumeTraversalImminent;
	FVector entryPoint = Origin, previousEntryPoint;

	for (int32 i = 0; i < VolumeSolution.Num();)
	{
		previousEntryPoint = entryPoint;
		entryPoint = NavEntryPointFromPath(previousEntryPoint, Goal, i, VolumeSolution, VolumeTraversalImminent, i, DrawDebugVolumes);
		pathSolution.Add(entryPoint);
	}

	return pathSolution;
}

//

void DisplayDebugVolume(UDoNNavigationVolumeComponent* volume, FColor color)
{
	if (!volume)
		return;

	volume->SetVisibility(true);
	volume->SetHiddenInGame(false);
	volume->ShapeColor = color;
}

void HideDebugVolume(UDoNNavigationVolumeComponent* volume, bool DrawDebug = true)
{
	if (!volume || !DrawDebug)
		return;

	volume->SetHiddenInGame(true);
}

// DEPRECATED
void ADEPRECATED_VolumeAdaptiveBuilder::ExpandFrontierTowardsTarget(UDoNNavigationVolumeComponent* current, UDoNNavigationVolumeComponent* neighbor, DoNNavigation::PriorityQueue<UDoNNavigationVolumeComponent*> &frontier, TMap<UDoNNavigationVolumeComponent*, FVector> &entryPointMap, bool &goalFound, UDoNNavigationVolumeComponent* start, UDoNNavigationVolumeComponent* goal, FVector origin, FVector destination, TMap<UDoNNavigationVolumeComponent*, int>& VolumeVsCostMap, bool DrawDebug, TMap<UDoNNavigationVolumeComponent*, TArray<UDoNNavigationVolumeComponent*>> &PathVolumeSolutionMap)
{		
	if (DrawDebug)
	{		
		DisplayDebugVolume(current, FColor::Red);
		DisplayDebugVolume(neighbor, FColor::Blue);
	}
	float SegmentDist = 0;
	FVector nextEntryPoint;

	TArray<UDoNNavigationVolumeComponent*> PathSolutionSoFar = PathVolumeSolutionMap.FindOrAdd(current);
	
	nextEntryPoint = NavEntryPointsForTraversal(*entryPointMap.Find(current), current, neighbor, SegmentDist, DrawDebug);

	entryPointMap.Add(neighbor, nextEntryPoint);

	if (nextEntryPoint == *entryPointMap.Find(current)) // i.e. no traversal solution exists
	{
		if (DrawDebug)
		{
			DisplayDebugVolume(current, FColor::Red);
			DisplayDebugVolume(neighbor, FColor::Blue);
		}

		UE_LOG(LogTemp, Log, TEXT("Skipping neighbor due to lack of traversal solution"));
		return;
	}

	//int new_cost = *VolumeVsCostMap.Find(current) + graph.cost(current, next);
	int new_cost = *VolumeVsCostMap.Find(current) + SegmentDist;

	if (!VolumeVsCostMap.Contains(neighbor) || new_cost < *VolumeVsCostMap.Find(neighbor))
	{	
		PathSolutionSoFar.Add(neighbor);
		PathVolumeSolutionMap.Add(neighbor, PathSolutionSoFar);
		VolumeVsCostMap.Add(neighbor, new_cost);

		float heuristic = FVector::Dist(nextEntryPoint, destination);
		int priority = new_cost + heuristic;

		if (DrawDebug)
		{
			DrawDebugLine(GetWorld(), nextEntryPoint, destination, FColor::Red, true, -1.f, 0, 10.f);
			FString priorityText = FString::Printf(TEXT("Priority: %d"), priority);
			UE_LOG(LogTemp, Log, TEXT("%s"), *priorityText);
		}			

		frontier.put(neighbor, priority);		
	}
}

// DEPRECATED
TArray<UDoNNavigationVolumeComponent*> ADEPRECATED_VolumeAdaptiveBuilder::GetShortestPathToDestination(FVector origin, FVector destination, UDoNNavigationVolumeComponent* originVolume,
	UDoNNavigationVolumeComponent* destinationVolume, TArray<FVector> &PathSolutionRaw, TArray<FVector> &PathSolutionOptimized, 
	bool DrawDebugVolumes, bool VisualizeRawPath, bool VisualizeOptimizedPath, bool VisualizeInRealTime, float LineThickness)
{
	TArray<UDoNNavigationVolumeComponent*> path;	

	uint64 timer1 = DoNNavigation::Debug_GetTimer();

	if (!originVolume || !destinationVolume)
		return path;
	
	DoNNavigation::PriorityQueue<UDoNNavigationVolumeComponent*> frontier;	
	TMap<UDoNNavigationVolumeComponent*, int> VolumeVsCostMap;
	TMap<UDoNNavigationVolumeComponent*, FVector> entryPointMap;	
	TMap<UDoNNavigationVolumeComponent*, TArray<UDoNNavigationVolumeComponent*>> PathVolumeSolutionMap;	
	TArray <UDoNNavigationVolumeComponent*> originPathVolumes;
	bool goalFound = false;

	frontier.put(originVolume, 0);	
	VolumeVsCostMap.Add(originVolume, 0);
	entryPointMap.Add(originVolume, origin);			
	originPathVolumes.Add(originVolume);	
	PathVolumeSolutionMap.Add(originVolume, originPathVolumes);

	while (!frontier.empty())
	{
		auto current = frontier.get();

		if (current == destinationVolume)
		{	
			goalFound = true;
			break;
		}
		
		TArray<UDoNNavigationVolumeComponent*> neighbors = NavGraph.neighbors(current);		

		for (auto neighbor : neighbors)
		{
			ExpandFrontierTowardsTarget(current, neighbor, frontier, entryPointMap, goalFound, originVolume, destinationVolume, origin, destination, VolumeVsCostMap, false, PathVolumeSolutionMap);
		}	
	}

	if (!goalFound)
		UE_LOG(LogTemp, Log, TEXT("NAV Engine error: Goal not found for routine 'GetShortestPathToDestination'"));

	if (PathVolumeSolutionMap.Find(destinationVolume))
		path = *PathVolumeSolutionMap.Find(destinationVolume);
	else
	{
		FString noSolLol = FString::Printf(TEXT("Path Solution Map does not contain destination volume among %d solution volumes. Could not solve path traversal for the requested path'"), PathVolumeSolutionMap.Num());
		UE_LOG(LogTemp, Log, TEXT("%s"), *noSolLol);
	}
		

	DoNNavigation::Debug_StopTimer(timer1);
	FString calcTime1 = FString::Printf(TEXT("Time spent calculating best path volumes - %f seconds"), timer1 / 1000.0);
	UE_LOG(LogTemp, Log, TEXT("%s"), *calcTime1);

	timer1 = DoNNavigation::Debug_GetTimer();
	PathSolutionRaw = PathSolutionFromVolumeSolution(path, origin, destination, DrawDebugVolumes);
	DoNNavigation::Debug_StopTimer(timer1);
	FString calcTime2 = FString::Printf(TEXT("Time spent calculating solution path - %f seconds"), timer1 / 1000.0);
	UE_LOG(LogTemp, Log, TEXT("%s"), *calcTime2);

	timer1 = DoNNavigation::Debug_GetTimer();
	OptimizePathSolution(PathSolutionRaw, PathSolutionOptimized);
	DoNNavigation::Debug_StopTimer(timer1);
	FString calcTime3 = FString::Printf(TEXT("Time spent optimizing path solution - %f seconds"), timer1 / 1000.0);
	UE_LOG(LogTemp, Log, TEXT("%s"), *calcTime3);

	VisualizeSolution(origin, destination, PathSolutionRaw, PathSolutionOptimized, VisualizeRawPath, VisualizeOptimizedPath, VisualizeInRealTime, DrawDebugVolumes, LineThickness);

	return path;
}

DoNNavigation::PriorityQueue<UDoNNavigationVolumeComponent*> frontier__;
UDoNNavigationVolumeComponent* neighbor__;
int neighborId__;
TMap<UDoNNavigationVolumeComponent*, UDoNNavigationVolumeComponent*> came_fron__;
TMap<UDoNNavigationVolumeComponent*, int> VolumeVsCostMap__;
TMap<UDoNNavigationVolumeComponent*, FVector> entryPointMap__;
TMap<UDoNNavigationVolumeComponent*, TArray<UDoNNavigationVolumeComponent*>> PathVolumeSolutionMap__;
UDoNNavigationVolumeComponent* current__;
bool goalFound__ = false;

void ADEPRECATED_VolumeAdaptiveBuilder::GetShortestPathToDestination_DebugRealtime(bool Reset, FVector origin, FVector destination, UDoNNavigationVolumeComponent* originVolume, UDoNNavigationVolumeComponent* destinationVolume, bool DrawDebug, TArray<FVector> &PathSolutionRaw, TArray<FVector> &PathSolutionOptimized, bool DrawDebugVolumes)
{
	if (!originVolume || !destinationVolume)
		return;

	if (goalFound__)
	{
		UE_LOG(LogTemp, Log, TEXT("Goal already reached."));
		return;
	}	

	if (Reset)
	{	
		goalFound__ = false;
		frontier__.put(originVolume, 0);
		//came_fron__.Add(originVolume, originVolume);
		VolumeVsCostMap__.Add(originVolume, 0);
		entryPointMap__.Add(originVolume, origin);
		neighborId__ = 0;
		neighbor__ = NULL;		
	}	

	if (!frontier__.empty())
	{	
		if (neighborId__ == 0)
		{
			current__ = frontier__.get();
			if (DrawDebug)
			{
				FlushPersistentDebugLines(GetWorld());
				//DrawDebugLine(GetWorld(), *entryPointMap__.Find(current__), *entryPointMap__.Find(*came_fron__.Find(current__)), FColor::Green, true, -1.f, 0, 10.f);
			}
			
		}
			
		
		if (NavGraph.neighbors(current__).Num() == 0)
			return;
		
		HideDebugVolume(neighbor__, DrawDebug);

		neighbor__ = NavGraph.neighbors(current__)[neighborId__];

		ExpandFrontierTowardsTarget(current__, neighbor__, frontier__, entryPointMap__, goalFound__, originVolume, destinationVolume, origin, destination, VolumeVsCostMap__, DrawDebug, PathVolumeSolutionMap__);

		if (neighborId__ == NavGraph.neighbors(current__).Num() - 1)
			neighborId__ = 0;
		else
		{
			neighborId__++;
			HideDebugVolume(current__, DrawDebug);
		}
			
	}
}

void ADEPRECATED_VolumeAdaptiveBuilder::VisualizeNAVResult(const TArray<FVector>& PathSolution, FVector Source, FVector Destination, bool Reset, bool DrawDebugVolumes, FColor LineColor, float LineThickness)
{
	FVector entryPoint = Source;
	FVector previousEntryPoint;

	for (int32 i = 0; i < PathSolution.Num(); i++)
	{
		previousEntryPoint = entryPoint;
		entryPoint = PathSolution[i];

		DrawDebugLine(GetWorld(), previousEntryPoint, entryPoint, LineColor, true, -1.f, 5.f, LineThickness);
		DrawDebugPoint(GetWorld(), entryPoint, 10.f, FColor::Blue, true);
	}
}

int32 i_viz;
FVector EntryPoint__;
FVector PreviousEntryPoint__;

void ADEPRECATED_VolumeAdaptiveBuilder::VisualizeNAVResultRealTime(const TArray<FVector>& PathSolution, FVector Source, FVector Destination, bool Reset, bool DrawDebugVolumes, FColor LineColor, float LineThickness)
{
	if (Reset)
	{
		i_viz = 0;		
		EntryPoint__ = Source;
		FlushPersistentDebugLines(GetWorld());
		DrawDebugPoint(GetWorld(), Source, 10.f, FColor::Blue, true);
		return;
	}

	if (i_viz > PathSolution.Num() - 1)
		return;

	PreviousEntryPoint__ = EntryPoint__;
	EntryPoint__ = PathSolution[i_viz];
	DrawDebugLine(GetWorld(), PreviousEntryPoint__, EntryPoint__, LineColor, true, -1.f, 5.f, LineThickness);
	DrawDebugPoint(GetWorld(), EntryPoint__, 10.f, FColor::Blue, true);
	
	return;
}

void ADEPRECATED_VolumeAdaptiveBuilder::VisualizeSolution(FVector source, FVector destination, const TArray<FVector>& PathSolutionRaw, const TArray<FVector>& PathSolutionOptimized, bool VisualizeRawPath, bool VisualizeOptimizedPath, bool VisualizeInRealTime, bool DrawDebugVolumes, float LineThickness)
{
	//FlushPersistentDebugLines(GetWorld());

	if (VisualizeRawPath)
	{
		if (VisualizeInRealTime)
			ADEPRECATED_VolumeAdaptiveBuilder::VisualizeNAVResultRealTime(PathSolutionRaw, source, destination, true, DrawDebugVolumes, FColor::Red, LineThickness);
		else
			ADEPRECATED_VolumeAdaptiveBuilder::VisualizeNAVResult(PathSolutionRaw, source, destination, true, DrawDebugVolumes, FColor::Red, LineThickness);
	}

	if (VisualizeOptimizedPath)
	{
		if (VisualizeInRealTime)
			ADEPRECATED_VolumeAdaptiveBuilder::VisualizeNAVResultRealTime(PathSolutionOptimized, source, destination, true, DrawDebugVolumes, FColor::Green, LineThickness);
		else
			ADEPRECATED_VolumeAdaptiveBuilder::VisualizeNAVResult(PathSolutionOptimized, source, destination, true, DrawDebugVolumes, FColor::Green, LineThickness);
	}
	
}

FVector ADEPRECATED_VolumeAdaptiveBuilder::NavEntryPointsForTraversal(FVector Origin, UDoNNavigationVolumeComponent* CurrentVolume, UDoNNavigationVolumeComponent* DestinationVolume, float &SegmentDist, bool DrawDebug)
{
	bool volumeTraversalmminent = false, overlapsX, overlapsY, overlapsZ;
	FVector entryPoint = Origin;
	FVector previousEntryPoint;
	int32 maxTries = 5, numTries = 0;

	while (!volumeTraversalmminent && numTries < maxTries)
	{
		previousEntryPoint = entryPoint;
		entryPoint = NavigaitonEntryPointFromVector(previousEntryPoint, CurrentVolume, DestinationVolume, volumeTraversalmminent, overlapsX, overlapsY, overlapsZ, false);
		float subSegmentDist = FVector::Dist(previousEntryPoint, entryPoint);
		SegmentDist += subSegmentDist;

		if (DrawDebug)
			DrawDebugLine(GetWorld(), previousEntryPoint, entryPoint, FColor::Green, true, -1.f, 0, 10.f);

		numTries++;
	}

	return entryPoint;
}

//float offset = 150 / 10;
float offset = 150 / 6;
//float offset = 150 / 4;
//float offset = 150/2;
float offsetOverlapTolerance = 20/2; // Ideally this needs to be tied to the 'minimum proximity required' setting of the 'Fly via best path' pollinator BT task

bool VolumesOverlapAxis(float aMin, float aMax, float bMin, float bMax)
{ 
	bool noOverlap = (aMin < bMin && aMax < bMin) || (aMin > bMax && aMax > bMax);
	return !noOverlap;
};

float GetBoxExtremityForAxis(char axis, int minOrMax, UDoNNavigationVolumeComponent* volume)
{
	switch (axis)
	{
	case 'X':
		return volume->Bounds.GetBoxExtrema(minOrMax).X;
	case 'Y':
		return volume->Bounds.GetBoxExtrema(minOrMax).Y;
	case 'Z':
		return volume->Bounds.GetBoxExtrema(minOrMax).Z;
	default:
		return 0;
	}
}

float NearestAxisPoint(char axis, float l, UDoNNavigationVolumeComponent* current, UDoNNavigationVolumeComponent* destination, bool inversed)
{
	int aMin = GetBoxExtremityForAxis(axis, 0, current);
	int aMax = GetBoxExtremityForAxis(axis, 1, current);
	int bMin = GetBoxExtremityForAxis(axis, 0, destination);
	int bMax = GetBoxExtremityForAxis(axis, 1, destination);

	bool shouldSnapToNextVolume = !VolumesOverlapAxis(aMin, aMax, bMin, bMax);

	if (inversed)
		shouldSnapToNextVolume = !shouldSnapToNextVolume;

	if (shouldSnapToNextVolume && l < bMin + (offset - offsetOverlapTolerance))  // snap to min bounds?
		return bMin + offset;
	else
		if (shouldSnapToNextVolume && l > bMax - (offset - offsetOverlapTolerance)) // snap to max bounds?
			return bMax - offset;
		else
			return l; // retain position - either if already overlapping or if no appropriate volume overlap is available yet
};

bool PointOverlapsVolumeAxis(char axis, FVector point, UDoNNavigationVolumeComponent* volume)
{
	// For reference:
	// minX = volume->Bounds.GetBoxExtrema(0).X == volume->GetComponentLocation().X - volume->GetUnscaledBoxExtent().X;
	// maxX = volume->Bounds.GetBoxExtrema(1).X == volume->GetComponentLocation().X + volume->GetUnscaledBoxExtent().X;

	switch (axis)
	{
	case 'X':
		return point.X > volume->Bounds.GetBoxExtrema(0).X + (offset - offsetOverlapTolerance)
			&& point.X < volume->Bounds.GetBoxExtrema(1).X - (offset - offsetOverlapTolerance);
		break;
	case 'Y':
		return point.Y > volume->Bounds.GetBoxExtrema(0).Y + (offset - offsetOverlapTolerance)
			&& point.Y < volume->Bounds.GetBoxExtrema(1).Y - (offset - offsetOverlapTolerance);
	case 'Z':
		return point.Z > volume->Bounds.GetBoxExtrema(0).Z + (offset - offsetOverlapTolerance)
			&& point.Z < volume->Bounds.GetBoxExtrema(1).Z - (offset - offsetOverlapTolerance);
	default:
		return false;
	}
}

bool PointOverlapsNeighborsAlongCongruentAxes(FVector point, TArray<UDoNNavigationVolumeComponent*> currentVolumeNeighbors, TArray<UDoNNavigationVolumeComponent*> destinationVolumeNeighbors, UDoNNavigationVolumeComponent* destination, bool currentVolOverlapsDestX, bool currentVolOverlapsDestY, bool currentVolOverlapsDestZ)
{
	bool overlaps = false;

	for (UDoNNavigationVolumeComponent* neighbor : currentVolumeNeighbors)
	{
		if (neighbor == destination)
			continue;

		bool overlapsX = PointOverlapsVolumeAxis('X', point, neighbor);
		bool overlapsY = PointOverlapsVolumeAxis('Y', point, neighbor);
		bool overlapsZ = PointOverlapsVolumeAxis('Z', point, neighbor);

		if ((overlapsX == true && currentVolOverlapsDestX == true && overlapsY == true && currentVolOverlapsDestY == true) ||
			(overlapsX == true && currentVolOverlapsDestX == true && overlapsZ == true && currentVolOverlapsDestZ == true) ||
			(overlapsY == true && currentVolOverlapsDestY == true && overlapsZ == true && currentVolOverlapsDestZ == true)
			)
		{
			if (!destinationVolumeNeighbors.Contains(neighbor))
				continue;
			
			/*
			neighbor->ShapeColor = FColor::Red;
			neighbor->SetVisibility(true);
			neighbor->SetHiddenInGame(false);
			*/
			overlaps = true;
			break;
		}		
	}

	return overlaps;
}

FVector ADEPRECATED_VolumeAdaptiveBuilder::NavigaitonEntryPointFromVector(FVector Origin, UDoNNavigationVolumeComponent* CurrentVolume, UDoNNavigationVolumeComponent* DestinationVolume, bool &VolumeTraversalImminent, bool &overlapsX, bool &overlapsY, bool &overlapsZ, bool DrawDebug)
{	
	// Debug info:
	if (DrawDebug)
	{
		CurrentVolume->SetVisibility(true);
		CurrentVolume->SetHiddenInGame(false);
		DestinationVolume->SetVisibility(true);
		DestinationVolume->SetHiddenInGame(false);
		DrawDebugPoint(GetWorld(), DestinationVolume->Bounds.GetBoxExtrema(0), 10.f, FColor::Blue, true);
		DrawDebugPoint(GetWorld(), DestinationVolume->Bounds.GetBoxExtrema(1), 10.f, FColor::Red, true);
	}

	overlapsX = PointOverlapsVolumeAxis('X', Origin, DestinationVolume);
	overlapsY = PointOverlapsVolumeAxis('Y', Origin, DestinationVolume);
	overlapsZ = PointOverlapsVolumeAxis('Z', Origin, DestinationVolume);
	VolumeTraversalImminent = (overlapsX && overlapsY) || (overlapsY && overlapsZ) || (overlapsZ && overlapsX);

	float destinationX = NearestAxisPoint('X', Origin.X, CurrentVolume, DestinationVolume, (!VolumeTraversalImminent && !overlapsX));
	float destinationY = NearestAxisPoint('Y', Origin.Y, CurrentVolume, DestinationVolume, (!VolumeTraversalImminent && !overlapsY));
	float destinationZ = NearestAxisPoint('Z', Origin.Z, CurrentVolume, DestinationVolume, (!VolumeTraversalImminent && !overlapsZ));		

	return FVector(destinationX, destinationY, destinationZ);
}

FVector ADEPRECATED_VolumeAdaptiveBuilder::NavigaitonEntryPoint(AActor* Actor, UDoNNavigationVolumeComponent* CurrentVolume, UDoNNavigationVolumeComponent* DestinationVolume, bool &VolumeTraversalImminent, bool &overlapsX, bool &overlapsY, bool &overlapsZ)
{
	return NavigaitonEntryPointFromVector(Actor->GetActorLocation(), CurrentVolume, DestinationVolume, VolumeTraversalImminent, overlapsX, overlapsY, overlapsZ, false);
}

FVector ADEPRECATED_VolumeAdaptiveBuilder::NavEntryPointFromPath(FVector Origin, FVector FinalDestination, int32 currentVolumeIndex, TArray<UDoNNavigationVolumeComponent*> path, bool &VolumeTraversalImminent, int32 &newVolumeIndex, bool DrawDebugVolumes)
{
	if (currentVolumeIndex >= path.Num() - 1)
	{
		newVolumeIndex = path.Num();
		return FinalDestination; 
	}
		

	bool VolumeTraversalmminent2, overlapsX, overlapsY, overlapsZ, overlapsX2, overlapsY2, overlapsZ2;	
	newVolumeIndex = currentVolumeIndex;

	FVector bestEntryPoint = NavigaitonEntryPointFromVector(Origin, path[currentVolumeIndex], path[currentVolumeIndex + 1], VolumeTraversalImminent, overlapsX, overlapsY, overlapsZ, DrawDebugVolumes);

	if (!VolumeTraversalImminent)
		return bestEntryPoint; // intra-zone entry point	

	while (VolumeTraversalImminent)
	{
		newVolumeIndex++;

		if (newVolumeIndex == path.Num() - 1)
		{	
			FVector nextEntryPointToV2 = NavigaitonEntryPointFromVector(FinalDestination, path[newVolumeIndex], path[newVolumeIndex-1], VolumeTraversalImminent, overlapsX2, overlapsY2, overlapsZ2, DrawDebugVolumes);
			if (VolumeTraversalImminent && overlapsX == overlapsX2 && overlapsY == overlapsY2 && overlapsZ == overlapsZ2)
			//if (PointOverlapsNeighborsAlongCongruentAxes(FinalDestination, NavGraph.neighbors(path[newVolumeIndex]), NavGraph.neighbors(path[newVolumeIndex-1]), path[newVolumeIndex-1], overlapsX, overlapsY, overlapsZ))
			{
				newVolumeIndex++; // TBD - needs overlap testing? Imagine bird's eye o-->|OBSTACLE|d
				return FinalDestination;
			}
			else
				return bestEntryPoint;
		}
		
		FVector nextEntryPointToV2 = NavigaitonEntryPointFromVector(Origin, path[currentVolumeIndex], path[newVolumeIndex + 1], VolumeTraversalImminent, overlapsX2, overlapsY2, overlapsZ2, DrawDebugVolumes);
		if (VolumeTraversalImminent && overlapsX == overlapsX2 && overlapsY == overlapsY2 && overlapsZ == overlapsZ2)
		{
			bestEntryPoint = nextEntryPointToV2;
			
		}
		else
		{
			FVector nextIntraZonePoint = NavigaitonEntryPointFromVector(bestEntryPoint, path[newVolumeIndex], path[newVolumeIndex + 1], VolumeTraversalmminent2, overlapsX2, overlapsY2, overlapsZ2, false);

			if (PointOverlapsNeighborsAlongCongruentAxes(nextIntraZonePoint, NavGraph.neighbors(path[currentVolumeIndex]), NavGraph.neighbors(path[newVolumeIndex]), path[newVolumeIndex], overlapsX, overlapsY, overlapsZ))
				return nextIntraZonePoint; // current volume has direct access to the intra-zone point between the next two volumes - i.e i+1 and i+2

			FVector bestEntryPointToNextIntraZonePoint = NavigaitonEntryPointFromVector(nextIntraZonePoint, path[newVolumeIndex], path[currentVolumeIndex], VolumeTraversalmminent2, overlapsX, overlapsY, overlapsZ, false);

			if (FVector::Dist(bestEntryPointToNextIntraZonePoint, nextIntraZonePoint) < FVector::Dist(bestEntryPoint, nextIntraZonePoint))
				return bestEntryPointToNextIntraZonePoint;
			else
				return bestEntryPoint;
		}
	}

	return bestEntryPoint;
}
	
void ADEPRECATED_VolumeAdaptiveBuilder::DisplayGraphDebugInfo()
{
		
		#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Completed building graph, output:"));
		for (const auto& Entry : NavGraph.nodes)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Edges for volume: ") + (*NavGraph.volumeLookupDictionary.Find(Entry.Key))->GetActorLabel());
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Edges for volume: ") + Entry.Key->GetFullName());

			for (UDoNNavigationVolumeComponent* volume : Entry.Value)
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("-------------> ") + volume->GetFullName());
		}
		#endif
}

UDoNNavigationVolumeComponent* PickVolumeFromComponents(TArray<UPrimitiveComponent*> overlappingComponents)
{
	for (UPrimitiveComponent* component : overlappingComponents)
	{
		UDoNNavigationVolumeComponent* volume = Cast<UDoNNavigationVolumeComponent>(component);
		if (volume)
			return volume;
	}

	return NULL;
}

UDoNNavigationVolumeComponent* ADEPRECATED_VolumeAdaptiveBuilder::GetNAVVolumeFromComponent(UPrimitiveComponent* Component, FVector ComponentAt)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> NAVOverlapQuery;
	TArray<AActor*> ActorsToIgnoreForCollision;
	TArray<UPrimitiveComponent*> overlappingComponents;
	NAVOverlapQuery.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));

	FTransform transform = FTransform(ComponentAt);

	UKismetSystemLibrary::ComponentOverlapComponents_NEW(Component, transform, NAVOverlapQuery, UDoNNavigationVolumeComponent::StaticClass(), ActorsToIgnoreForCollision, overlappingComponents);

	return PickVolumeFromComponents(overlappingComponents);
}

UDoNNavigationVolumeComponent* ADEPRECATED_VolumeAdaptiveBuilder::GetNAVVolumeFromObject(UObject* Actor)
{
	if (UDoNNavigationVolumeComponent::StaticClass() == Actor->StaticClass())
		return Cast<UDoNNavigationVolumeComponent>(Actor);
	
	AActor* actor = Cast<AActor>(Actor);

	if (!actor)
		return NULL;

	TArray<UPrimitiveComponent*> overlappingComponents;
	actor->GetOverlappingComponents(overlappingComponents);	

	return PickVolumeFromComponents(overlappingComponents);

}

