// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoxelTestGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "VoxelTestCharacter.h"

AVoxelTestGameMode::AVoxelTestGameMode() {
    // set default pawn class to our Blueprinted character
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    if (PlayerPawnBPClass.Class != NULL) {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}
