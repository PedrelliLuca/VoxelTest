// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoxelTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MatterShapingComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AVoxelTestCharacter::AVoxelTestCharacter() {
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;            // Character moves in the direction of input...
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;       // The camera follows at this distance behind the character
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(
        CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false;    // Camera does not rotate relative to arm

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AVoxelTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoxelTestCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoxelTestCharacter::Look);

        // DrawAdd
        EnhancedInputComponent->BindAction(DrawAddAction, ETriggerEvent::Triggered, this, &AVoxelTestCharacter::_addCube);

        // DrawRemove
        EnhancedInputComponent->BindAction(DrawRemoveAction, ETriggerEvent::Triggered, this, &AVoxelTestCharacter::_removeCube);
    } else {
        UE_LOG(LogTemplateCharacter, Error,
            TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy "
                 "system, then you will need to update this C++ file."),
            *GetNameSafe(this));
    }
}

void AVoxelTestCharacter::Move(FInputActionValue const& Value) {
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr) {
        // find out which way is forward
        FRotator const Rotation = Controller->GetControlRotation();
        FRotator const YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        FVector const ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // get right vector
        FVector const RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AVoxelTestCharacter::Look(FInputActionValue const& Value) {
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr) {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AVoxelTestCharacter::BeginPlay() {
    // Call the base class
    Super::BeginPlay();

    _matterShapingComponent = FindComponentByClass<UMatterShapingComponent>();
    check(_matterShapingComponent.IsValid());
}

void AVoxelTestCharacter::_addCube(FInputActionValue const& value) {
    auto shapingRequest = FMatterShapingRequest();
    _shapingRequestFromCameraLineTrace(shapingRequest);
    _matterShapingComponent->ShapeMatter(shapingRequest, EShapingOperation::Add);
}

void AVoxelTestCharacter::_removeCube(FInputActionValue const& value) {
    auto shapingRequest = FMatterShapingRequest();
    _shapingRequestFromCameraLineTrace(shapingRequest);
    _matterShapingComponent->ShapeMatter(shapingRequest, EShapingOperation::Remove);
}

void AVoxelTestCharacter::_shapingRequestFromCameraLineTrace(FMatterShapingRequest& outShapingRequest) const {
    FCollisionQueryParams queryParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
    queryParams.bTraceComplex = true;
    queryParams.bReturnPhysicalMaterial = false;

    // Re-initialize hit info
    auto hitResult = FHitResult(ForceInit);

    // call GetWorld() from within an actor extending class
    auto const startRay = FVector(FollowCamera->GetComponentLocation());
    auto const endRay = FVector(startRay + FollowCamera->GetForwardVector() * 3000.f);
    GetWorld()->LineTraceSingleByChannel(hitResult, // result
        startRay,                                // start
        endRay,                                  // end
        ECC_Pawn,                                // collision channel
        queryParams);

    DrawDebugLine(GetWorld(), startRay, endRay, FColor::Red, false, 5.f);

    outShapingRequest = FMatterShapingRequest(hitResult.ImpactPoint, 200.f, hitResult.GetActor());
}
