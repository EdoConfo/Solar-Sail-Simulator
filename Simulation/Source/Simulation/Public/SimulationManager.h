#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimulationManager.generated.h"

UCLASS()
class SIMULATION_API ASimulationManager : public AActor {
	GENERATED_BODY()
	
public:	
	ASimulationManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

    /* Space Physics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics")                                                   float TimeScale;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics")                                                   float ForceMultiplier;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics", meta = (ClampMin = "1.0e6", ClampMax = "3.0e13")) double SunDistanceCm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Space Physics")                                                 float SolarPressureAt1AU;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Space Physics")                                                 float AuToUnrealScale;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Space Physics")                                                 double GravitationalConstant = 6.67430e-11;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Space Physics")                                                 double EarthMass = 5.972e24;
    
    /* Sail Configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration")                                                         float SailMass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration")                                                         float SailArea;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration", meta = (ClampMin = "1", ClampMax = "100"))              int32 GridResolution;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration", meta = (ClampMin = "6371000", ClampMax = "1500000000")) double InitialSailDistanceKm = 35786.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration")                                                         bool bIsDoubleSided;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration")                                                         bool bShowPhotonDebug;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration")                                                         bool bShowSunDistanceDebug;

    static ASimulationManager* Instance;
};
