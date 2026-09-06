// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "PhysicsSimComponent.h"

UPhysicsSimComponent::UPhysicsSimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


//comment test

void UPhysicsSimComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	SimMeshComponent = Owner->FindComponentByClass<UPrimitiveComponent>();
	if (!SimMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("PhysicsSimComponent: no UPrimitiveComponent found on owner."));
		return;
	}

	// Sync radius from the actual mesh bounds (cm -> meters)
	const float RealRadiusCm = SimMeshComponent->Bounds.SphereRadius;
	Profile.Radius = RealRadiusCm / 100.0f;
	UE_LOG(LogTemp, Warning, TEXT("Synced Profile.Radius from mesh bounds: %.4f m"), Profile.Radius);

	CachedBodyInstance = SimMeshComponent->GetBodyInstance();
	if (!CachedBodyInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("PhysicsSimComponent: owner's primitive has no BodyInstance."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("BodyInstance found. bSimulatePhysics=%d"), CachedBodyInstance->bSimulatePhysics);

	// Sync mass from Chaos
	Profile.Mass = CachedBodyInstance->GetBodyMass();
	UE_LOG(LogTemp, Warning, TEXT("Synced Profile.Mass from Chaos: %.4f kg"), Profile.Mass);
}




FVector UPhysicsSimComponent::ComputeDragForce(const FVector& VelocityCmS) const
{
	// Convert velocity from UE units (cm/s) to SI (m/s)
	const FVector VelocitySI = VelocityCmS / 100.0f;
	const float SpeedSI = VelocitySI.Size();

	if (SpeedSI < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	constexpr float AirDensity = 1.225f; // kg/m^3, sea level, placeholder
	const float Area = PI * FMath::Square(Profile.Radius); // m^2, assuming Profile.Radius is in meters

	// Standard drag equation: F = -0.5 * rho * Cd * A * |v| * v
	const FVector DragForceSI = -0.5f * AirDensity * Profile.DragCoefficient * Area * SpeedSI * VelocitySI;

	// Convert force back to UE's native units (kg * cm/s^2)
	return DragForceSI * 100.0f;
}

FVector UPhysicsSimComponent::ComputeMagnusForce(const FVector& VelocityCmS, const FVector& AngularVelocityRadS) const
{
	const FVector VelocitySI = VelocityCmS / 100.0f;
	// AngularVelocityRadS needs no conversion: rad/s is already SI

	constexpr float AirDensity = 1.225f; // kg/m^3, same as drag
	const float Area = PI * FMath::Square(Profile.Radius);

	// F = 0.5 * rho * Cl * A * (omega x v)
	const FVector MagnusForceSI = 0.5f * AirDensity * Profile.MagnusCoefficient * Area
		* FVector::CrossProduct(AngularVelocityRadS, VelocitySI);

	// Convert back to UE's native units (kg * cm/s^2)
	return MagnusForceSI * 100.0f;
}



void UPhysicsSimComponent::CustomPhysics(float DeltaTime, FBodyInstance* BodyInstance)
{
	if (!BodyInstance)
	{
		return;
	}

	CurrentState.Position = BodyInstance->GetUnrealWorldTransform().GetLocation();
	CurrentState.Velocity = BodyInstance->GetUnrealWorldVelocity();
	CurrentState.AngularVelocity = BodyInstance->GetUnrealWorldAngularVelocityInRadians();

	const FVector DragForce = ComputeDragForce(CurrentState.Velocity);
	const FVector MagnusForce = ComputeMagnusForce(CurrentState.Velocity, CurrentState.AngularVelocity);

	BodyInstance->AddForce(DragForce + MagnusForce, /*bAllowSubstepping=*/true, /*bAccelChange=*/false);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Magenta,
			FString::Printf(TEXT("Speed: %.1f cm/s | Lateral X: %.1f"), CurrentState.Velocity.Size(), CurrentState.Position.X));
	}
}




void UPhysicsSimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CachedBodyInstance)
	{
		return;
	}

	// Re-register every frame: AddCustomPhysics is consumed after each physics step,
	// it is NOT a persistent subscription.
	FCalculateCustomPhysics CustomPhysicsDelegate = FCalculateCustomPhysics::CreateUObject(this, &UPhysicsSimComponent::CustomPhysics);
	CachedBodyInstance->AddCustomPhysics(CustomPhysicsDelegate);
}
