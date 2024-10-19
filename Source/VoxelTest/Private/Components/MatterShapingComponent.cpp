// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MatterShapingComponent.h"

#include "VoxelTools/VoxelToolHelpers.h"
#include "VoxelWorld.h"

namespace VoxelUtilities {
FVoxelIntBox MakeVoxelIntBox(TObjectPtr<AVoxelWorld> const& voxelW, FVector const& center, float const side) {
    VOXEL_FUNCTION_COUNTER();
    CHECK_VOXELWORLD_IS_CREATED_IMPL(voxelW, {});

    auto const box = FBox(center - side, center + side);

    auto result = FVoxelIntBoxWithValidity();

    auto const transformVertex = [voxelW, &result](FVector const& vertex) {
        FVoxelVector const localPosition = voxelW->GlobalToLocalFloat(vertex);
        result += FVoxelUtilities::FloorToInt(localPosition);
        result += FVoxelUtilities::CeilToInt(localPosition);
    };

    // Here is the order in which vertices are added for a given Z.
    // First, the minZ side, then the maxZ one.
    //
    //                 X
    //                ^
    //  2             |              4
    //    +------------------------+
    //    |           |            |
    //    |           |            |
    //    |           |            |
    //    |           |            |
    //    |           |            |
    //----|------------------------|---->
    //    |           |            |     Y
    //    |           |            |
    //    |           |            |
    //    |           |            |
    //    |           |            |
    //    +------------------------+
    //  1             |             3
    //                |

    transformVertex({box.Min.X, box.Min.Y, box.Min.Z});
    transformVertex({box.Max.X, box.Min.Y, box.Min.Z});
    transformVertex({box.Min.X, box.Max.Y, box.Min.Z});
    transformVertex({box.Max.X, box.Max.Y, box.Min.Z});
    transformVertex({box.Min.X, box.Min.Y, box.Max.Z});
    transformVertex({box.Max.X, box.Min.Y, box.Max.Z});
    transformVertex({box.Min.X, box.Max.Y, box.Max.Z});
    transformVertex({box.Max.X, box.Max.Y, box.Max.Z});

    return result.GetBox();
}
} // namespace VoxelUtilities

void UMatterShapingComponent::ShapeMatter(FMatterShapingRequest const& shapingRequest) {
    // UVoxelBlueprintLibrary::TransformGlobalBoxToVoxelBox

    TObjectPtr<AVoxelWorld> const& matterVW = Cast<AVoxelWorld>(shapingRequest.MatterActor);
    if (!matterVW) {
        return;
    }

    VoxelUtilities::MakeVoxelIntBox(matterVW, shapingRequest.Location, shapingRequest.Side);
}
