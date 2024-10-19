// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MatterShapingComponent.h"

#include "VoxelTools/Gen/VoxelToolsBase.h"
#include "VoxelTools/Impl/VoxelBoxToolsImpl.h"
#include "VoxelTools/VoxelToolHelpers.h"
#include "VoxelWorld.h"

namespace VoxelUtilities {
FVoxelIntBox MakeVoxelIntBox(TObjectPtr<AVoxelWorld> const& voxelW, FVector const& center, float const side) {
    VOXEL_FUNCTION_COUNTER();
    CHECK_VOXELWORLD_IS_CREATED_IMPL(voxelW, FVoxelIntBox{});

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
    TObjectPtr<AVoxelWorld> const& voxelWorld = Cast<AVoxelWorld>(shapingRequest.MatterActor);
    if (!voxelWorld) {
        return;
    }

    FVoxelIntBox const shapeBound = VoxelUtilities::MakeVoxelIntBox(voxelWorld, shapingRequest.Location, shapingRequest.Side);

    // From UVoxelBoxTools::AddBox():
    // GENERATED_TOOL_FUNCTION(Value);
    VOXEL_FUNCTION_COUNTER();
    CHECK_VOXELWORLD_IS_CREATED_IMPL(voxelWorld.Get(), );

    // GENERATED_TOOL_CALL(Value, FVoxelBoxToolsImpl::AddBox(Data, Bounds))
    FVoxelData& worldData = voxelWorld->GetData();
    {
        auto const lock = FVoxelWriteScopeLock(worldData, shapeBound, FUNCTION_FNAME);

        // bMultiThreaded = true, bRecordModifiedValues = true
        auto data = TVoxelDataImpl<FModifiedVoxelValue>(worldData, true, true);
        FVoxelBoxToolsImpl::AddBox(data, shapeBound);

        // bUpdateRender = true
        FVoxelToolHelpers::UpdateWorld(voxelWorld, shapeBound);
    }
}
