// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "MatterShapingComponent.generated.h"

struct FMatterShapingRequest
{
    FVector Location = FVector();
    float Side = 0.0f;
    TObjectPtr<AActor> MatterActor = TObjectPtr<AActor>();
};

/**
 * \brief TODO
 */
UCLASS()
class VOXELTEST_API UMatterShapingComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMatterShapingComponent() = default;
    ~UMatterShapingComponent() override = default;


    void ShapeMatter(FMatterShapingRequest const& shapingRequest);

private:
};
