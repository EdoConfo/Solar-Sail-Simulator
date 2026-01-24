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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics") float SolarPressureAt1AU;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics") float AuToUnrealScale;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics") float TimeScale;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space Physics") float ForceMultiplier;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration") float SailMass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration") float SailArea;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration", meta = (ClampMin = "1", ClampMax = "100")) int32 GridResolution;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration") bool bShowPhotonDebug;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration") bool bShowSunDistanceDebug;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail Configuration") bool bIsDoubleSided;

    static ASimulationManager* Instance;
};
