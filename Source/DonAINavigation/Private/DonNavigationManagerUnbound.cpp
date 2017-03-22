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

#include "DonAINavigationPrivatePCH.h"

#include "DonNavigationManagerUnbound.h"

ADonNavigationManagerUnbound::ADonNavigationManagerUnbound(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsUnbound = true;
}

void ADonNavigationManagerUnbound::BeginPlay()
{
	Super::BeginPlay();
}

bool ADonNavigationManagerUnbound::PrepareSolution(FDonNavigationQueryTask& Task)
{
	if (!bIsUnbound)
		return Super::PrepareSolution(Task);

	auto& data = Task.Data;

	// a rare edgecase, but worth handling gracefully in any case
	if (data.OriginVolumeCenter == data.DestinationVolumeCenter)
	{
		data.PathSolutionRaw.Add(data.Origin);
		data.PathSolutionRaw.Add(data.Destination);

		return true;
	}

	// The trajectory map operates in reverse, so start from the destination:
	//VolumeSolution.Insert(DestinationVolume, 0);
	data.PathSolutionRaw.Insert(data.Destination, 0);

	// Work our way back from destination to origin while generating a linear path solution list
	bool originFound = false;
	auto nextLocation = data.VolumeVsGoalTrajectoryMap_Unbound.Find(data.DestinationVolumeCenter);

	while (nextLocation)
	{
		if (data.PathSolutionRaw.Contains(*nextLocation))
			break; // Needs more testing - this was introduced alongisde the bugfix to the bound manager to prevent infinte loops in PathSolutionFromVolumeTrajectoryMap

		//VolumeSolution.Insert(*nextLocation, 0);
		data.PathSolutionRaw.Insert((*nextLocation), 0);

		if (*nextLocation == data.OriginVolumeCenter)
		{
			originFound = true;
			break;
		}

		nextLocation = data.VolumeVsGoalTrajectoryMap_Unbound.Find(*nextLocation);
	}

	return originFound;
}


void ADonNavigationManagerUnbound::TickNavigationSolver(FDonNavigationQueryTask& task)
{
	if (!bIsUnbound)
	{
		Super::TickNavigationSolver(task);

		return;
	}

	auto& data = task.Data;

	data.SolverIterationCount++;

	if (!data.Frontier_Unbound.empty())
	{
		// Move towards goal by fetching the "best neighbor" of the previous volume from the Frontier priority queue
		// The best neighbor is defined as the node most likely to lead us towards the goal
		FVector currentVector = data.Frontier_Unbound.get();

		//UE_LOG(DoNNavigationLog, Warning, TEXT("[SOLVER] Current: %s"), *currentVector.ToString());

		// Have we reached the goal?
		//if (VolumeIdAt(currentVector) == VolumeIdAt(data.DestinationVolumeCenter))
		if (currentVector == data.DestinationVolumeCenter)
		{
			data.bGoalFound = true;
			return;
		}

		// Discover all neighbors for current volume:
		const auto& neighbors = NeighborsAsVectors(currentVector);

		// Evaluate each neighbor for suitability, assign points, add to Frontier
		for (auto neighbor : neighbors)
		{
			ExpandFrontierTowardsTarget(task, currentVector, neighbor);
		}
	}
}

TArray<FVector> ADonNavigationManagerUnbound::NeighborsAsVectors(FVector Location)
{
	TArray<FVector> neighbors;
	neighbors.Reserve(Volume6DOF + VolumeImplicitDOF);

	/*FVector volumeId = VolumeIdAt(Location);
	int x = volumeId.X, y = volumeId.Y, z = volumeId.Z;*/

	// 6 DOF neighbors (Direct neighbors)
	for (int32 i = 0; i < Volume6DOF; i++)
	{
		/*int32 xN = x + x6DOFCoords[i];
		int32 yN = y + y6DOFCoords[i];
		int32 zN = z + z6DOFCoords[i];

		neighbors.Add(LocationAtId(xN, yN, zN));*/

		FVector neighbor = Location + VoxelSize * FVector(x6DOFCoords[i], y6DOFCoords[i], z6DOFCoords[i]);

		neighbors.Add(neighbor);
	}

	// Implicit:
	
	// X		

	if (CanNavigate(LocationAtId(Location, 1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, 0, 1)))
		neighbors.Add(LocationAtId(Location, 1, 0, 1));

	if (CanNavigate(LocationAtId(Location, -1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, 0, 1)))
		neighbors.Add(LocationAtId(Location, -1, 0, 1));

	if (CanNavigate(LocationAtId(Location, 1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, 0, -1)))
		neighbors.Add(LocationAtId(Location, 1, 0, -1));

	if (CanNavigate(LocationAtId(Location, -1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, 0, -1)))
		neighbors.Add(LocationAtId(Location, -1, 0, -1));

	//Y
	if (CanNavigate(LocationAtId(Location, 0, 1, 0)) && CanNavigate(LocationAtId(Location, 0, 0, 1)))
		neighbors.Add(LocationAtId(Location, 0, 1, 1));

	if (CanNavigate(LocationAtId(Location, 0, -1, 0)) && CanNavigate(LocationAtId(Location, 0, 0, 1)))
		neighbors.Add(LocationAtId(Location, 0, -1, 1));

	if (CanNavigate(LocationAtId(Location, 0, 1, 0)) && CanNavigate(LocationAtId(Location, 0, 0, -1)))
		neighbors.Add(LocationAtId(Location, 0, 1, -1));

	if (CanNavigate(LocationAtId(Location, 0, -1, 0)) && CanNavigate(LocationAtId(Location, 0, 0, -1)))
		neighbors.Add(LocationAtId(Location, 0, -1, -1));

	//Z
	if (CanNavigate(LocationAtId(Location, 1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, 1, 0)))
		neighbors.Add(LocationAtId(Location, 1, 1, 0));

	if (CanNavigate(LocationAtId(Location, -1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, 1, 0)))
		neighbors.Add(LocationAtId(Location, -1, 1, 0));

	if (CanNavigate(LocationAtId(Location, 1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, -1, 0)))
		neighbors.Add(LocationAtId(Location, 1, -1, 0));

	if (CanNavigate(LocationAtId(Location, -1, 0, 0)) && CanNavigate(LocationAtId(Location, 0, -1, 0)))
		neighbors.Add(LocationAtId(Location, -1, -1, 0));

	// Note:- The commented code below is just a different way of performing the same operations done above.
	// Need to remove the commented code at some point, leaving it in there for now as the Unbound solution
	// might needs some time to mature.	

	// X		

	/*if (CanNavigate(LocationAtId(x + 1, y, z)) && CanNavigate(LocationAtId(x, y, z + 1)))
		neighbors.Add(LocationAtId(x + 1, y, z + 1));

	if (CanNavigate(LocationAtId(x - 1, y, z)) && CanNavigate(LocationAtId(x, y, z + 1)))
		neighbors.Add(LocationAtId(x - 1, y, z + 1));

	if (CanNavigate(LocationAtId(x + 1, y, z)) && CanNavigate(LocationAtId(x, y, z - 1)))
		neighbors.Add(LocationAtId(x + 1, y, z - 1));

	if (CanNavigate(LocationAtId(x - 1, y, z)) && CanNavigate(LocationAtId(x, y, z - 1)))
		neighbors.Add(LocationAtId(x - 1, y, z - 1));

	//Y
	if (CanNavigate(LocationAtId(x, y + 1, z)) && CanNavigate(LocationAtId(x, y, z + 1)))
		neighbors.Add(LocationAtId(x, y + 1, z + 1));

	if (CanNavigate(LocationAtId(x, y - 1, z)) && CanNavigate(LocationAtId(x, y, z + 1)))
		neighbors.Add(LocationAtId(x, y - 1, z + 1));

	if (CanNavigate(LocationAtId(x, y + 1, z)) && CanNavigate(LocationAtId(x, y, z - 1)))
		neighbors.Add(LocationAtId(x, y + 1, z - 1));

	if (CanNavigate(LocationAtId(x, y - 1, z)) && CanNavigate(LocationAtId(x, y, z - 1)))
		neighbors.Add(LocationAtId(x, y - 1, z - 1));

	//Z
	if (CanNavigate(LocationAtId(x + 1, y, z)) && CanNavigate(LocationAtId(x, y + 1, z)))
		neighbors.Add(LocationAtId(x + 1, y + 1, z));

	if (CanNavigate(LocationAtId(x - 1, y, z)) && CanNavigate(LocationAtId(x, y + 1, z)))
		neighbors.Add(LocationAtId(x - 1, y + 1, z));

	if (CanNavigate(LocationAtId(x + 1, y, z)) && CanNavigate(LocationAtId(x, y - 1, z)))
		neighbors.Add(LocationAtId(x + 1, y - 1, z));

	if (CanNavigate(LocationAtId(x - 1, y, z)) && CanNavigate(LocationAtId(x, y - 1, z)))
		neighbors.Add(LocationAtId(x - 1, y - 1, z));*/

	return neighbors;
}

void ADonNavigationManagerUnbound::ExpandFrontierTowardsTarget(FDonNavigationQueryTask& Task, FVector Current, FVector Neighbor)
{
	//DrawDebugBox(GetWorld(), Neighbor, FVector(VoxelSize/2, VoxelSize / 2, VoxelSize / 2), FColor::Green, true);
	//UE_LOG(DoNNavigationLog, Warning, TEXT("[SOLVER] Neighbor: %s"), *Neighbor.ToString());

	if (!CanNavigateByCollisionProfile(Neighbor, Task.Data.VoxelCollisionProfile))
	{
		//UE_LOG(DoNNavigationLog, Warning, TEXT("[SOLVER] Neighbor skipped"));
		return;
	}

	// In reality there are two possible segment distances: side and sqrt(2) * side. As a trade-off between accuracy and performance we're assuming all segments to be only equal to the pixel size (majority case are 6-DOF neighbors)
	float SegmentDist = VoxelSize; // Alternate: FVector::DistSquared(Current->Location, Neighbor->Location);

	uint32 newCost = *Task.Data.VolumeVsCostMap_Unbound.Find(Current) + SegmentDist;
	uint32* volumeCost = Task.Data.VolumeVsCostMap_Unbound.Find(Neighbor);

	if (!volumeCost || newCost < *volumeCost)
	{
		Task.Data.VolumeVsGoalTrajectoryMap_Unbound.Add(Neighbor, Current);
		Task.Data.VolumeVsCostMap_Unbound.Add(Neighbor, newCost);

		//UE_LOG(DoNNavigationLog, Warning, TEXT("[SOLVER] Neighbor new cost: %d"), newCost);

		float heuristic = FVector::Dist(Neighbor, Task.Data.Destination);
		uint32 priority = newCost + heuristic;

		Task.Data.Frontier_Unbound.put(Neighbor, priority);

		//UE_LOG(DoNNavigationLog, Warning, TEXT("[SOLVER] Neighbor new priority: %d"), priority);
	}
}