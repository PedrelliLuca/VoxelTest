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

//////////////////////////////////////////////////////////////////////////
// AVoxelTestCharacter

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

void AVoxelTestCharacter::DrawCube(FInputActionValue const& Value) {
    FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
    RV_TraceParams.bTraceComplex = true;
    RV_TraceParams.bReturnPhysicalMaterial = false;

    // Re-initialize hit info
    FHitResult RV_Hit(ForceInit);

    // call GetWorld() from within an actor extending class
    FVector const startRay{FollowCamera->GetComponentLocation()};
    FVector const endRay{startRay + FollowCamera->GetForwardVector() * 3000.};
    GetWorld()->LineTraceSingleByChannel(RV_Hit, // result
        startRay,                                // start
        endRay,                                  // end
        ECC_Pawn,                                // collision channel
        RV_TraceParams);

    DrawDebugLine(GetWorld(), startRay, endRay, FColor::Red, false, 5.);
    _matterShapingComponent->ShapeMatter({RV_Hit.ImpactPoint, 200., RV_Hit.GetActor()});
}

void AVoxelTestCharacter::BeginPlay() {
    // Call the base class
    Super::BeginPlay();

    _matterShapingComponent = FindComponentByClass<UMatterShapingComponent>();
    check(_matterShapingComponent.IsValid());
}

//////////////////////////////////////////////////////////////////////////
// Input

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

        // Draw
        EnhancedInputComponent->BindAction(DrawAction, ETriggerEvent::Triggered, this, &AVoxelTestCharacter::DrawCube);
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