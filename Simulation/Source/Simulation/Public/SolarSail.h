 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "SimulationManager.h"
#include "SolarSail.generated.h"

UCLASS()
class SIMULATION_API ASolarSail : public AActor {
	GENERATED_BODY()
	
public:
	ASolarSail();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Solar Sail") UStaticMeshComponent* SailMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Data") float SailArea;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Data") float TotalMass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Simulation Settings", meta = (ClampMin = "1", ClampMax = "100")) int32 GridResolution;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Simulation Settings") bool bShowPhotonDebug;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Simulation Settings") bool bShowSunDistanceDebug;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Simulation Settings") bool bIsDoubleSided;

private:
    float CachedSolarPressure;
    float CachedAuScale;

public:

    // TODO - DA TOGLIERE DOPO AVER CAPITO COME MODIFICARE IL TEMPO DI SIMULAZIONE IN UNREAL
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Data") float ForceMultiplier;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry") FVector CurrentSolarForce;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry") float CurrentIncidenceAngle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry") float CurrentDistanceAU;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System") ADirectionalLight* SunLightActor;

private:
    void FindSunInScene();
    void UpdateSolarForce(float DeltaTime);
};