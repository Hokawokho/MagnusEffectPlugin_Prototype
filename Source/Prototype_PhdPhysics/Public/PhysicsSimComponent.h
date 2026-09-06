// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsSimComponent.generated.h"



// ---------------------------------------------------------
// Data structs
// ---------------------------------------------------------

USTRUCT(BlueprintType)
struct FAeroState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    FVector AngularVelocity = FVector::ZeroVector; // spin (omega)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    float SeamOrientation = 0.0f; // gamma(t)
};



USTRUCT(BlueprintType)
struct FAeroProfile
{
    GENERATED_BODY()

    //PLACEHOLDERS  

    //Currently reading the mass from editor, calculated by Chaos Physic
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aero")
    float Mass = 0.0f;
    
    //Currently reading the radius from editor, calculated by Chaos Physic
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Aero")
    float Radius = 0.0f; // meters, 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    float DragCoefficient = 0.47f; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    float MagnusCoefficient = 1.0f; 
};

// ---------------------------------------------------------
// Component
// ---------------------------------------------------------

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROTOTYPE_PHDPHYSICS_API UPhysicsSimComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPhysicsSimComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    FAeroState CurrentState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aero")
    FAeroProfile Profile;

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // NEW: called by Chaos on every physics sub-step for the registered body
    void CustomPhysics(float DeltaTime, FBodyInstance* BodyInstance);

private:
    // NEW: cached reference to the primitive component whose body we hook into
    UPROPERTY()
    class UPrimitiveComponent* SimMeshComponent = nullptr;
    FBodyInstance* CachedBodyInstance = nullptr;
    
private:
    FVector ComputeDragForce(const FVector& VelocityCmS) const;    
    
private:
    FVector ComputeMagnusForce(const FVector& VelocityCmS, const FVector& AngularVelocityRadS) const;    
};