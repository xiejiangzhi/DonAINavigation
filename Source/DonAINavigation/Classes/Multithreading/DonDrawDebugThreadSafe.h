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

struct FDrawDebugLineRequest
{
	FVector LineStart;
	FVector LineEnd;
	FColor Color;
	bool bPersistentLines;
	float LifeTime;
	uint8 DepthPriority;
	float Thickness;

	FDrawDebugLineRequest() {}

	FDrawDebugLineRequest(FVector LineStart, FVector LineEnd, FColor Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
		: LineStart(LineStart), LineEnd(LineEnd), Color(Color), bPersistentLines(bPersistentLines), LifeTime(LifeTime), DepthPriority(DepthPriority), Thickness(Thickness)
	{

	}
};

struct FDrawDebugPointRequest
{
	FVector PointLocation;
	float PointThickness;
	FColor Color;
	bool bPersistentLines;
	float LifeTime;

	FDrawDebugPointRequest() {}

	FDrawDebugPointRequest(FVector PointLocation, float PointThickness, FColor Color, bool bPersistentLines, float LifeTime)
		: PointLocation(PointLocation), PointThickness(PointThickness), Color(Color), bPersistentLines(bPersistentLines), LifeTime(LifeTime)
	{

	}
};

struct FDrawDebugVoxelRequest
{
	FVector Center;
	FVector Box;
	FColor Color;
	bool bPersistentLines;
	float LifeTime;
	uint8 DepthPriority;
	float Thickness;

	FDrawDebugVoxelRequest() {}

	FDrawDebugVoxelRequest(FVector Center, FVector Box, FColor Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
		: Center(Center), Box(Box), Color(Color), bPersistentLines(bPersistentLines), LifeTime(LifeTime), DepthPriority(DepthPriority), Thickness(Thickness)
	{

	}
};

struct FDrawDebugSphereRequest
{
	FVector Center;
	float Radius;
	float Segments;
	FColor Color;
	bool bPersistentLines;
	float LifeTime;	

	FDrawDebugSphereRequest() {}

	FDrawDebugSphereRequest(FVector Center, float Radius, float Segments, FColor Color, bool bPersistentLines, float LifeTime)
		: Center(Center), Radius(Radius), Segments(Segments), Color(Color), bPersistentLines(bPersistentLines), LifeTime(LifeTime)
	{

	}
};