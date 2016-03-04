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


#include "Runtime/Engine/Classes/Components/BoxComponent.h"
#include "DEPRECATED_DoNNavigationVolumeComponent.generated.h"

/**
 * 
 */
UCLASS()
class  UDoNNavigationVolumeComponent : public UBoxComponent
{
	GENERATED_BODY() 

public:
	UDoNNavigationVolumeComponent(const FObjectInitializer& ObjectInitializer);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AerialNavigation")
	int32 X;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AerialNavigation")
	int32 Y;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AerialNavigation")
	int32 Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AerialNavigationVolume)
	bool CanNavigate;	
};
