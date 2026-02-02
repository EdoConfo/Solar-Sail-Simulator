#include "SimulationManager.h"

ASimulationManager* ASimulationManager::Instance = nullptr;

ASimulationManager::ASimulationManager() {
	PrimaryActorTick.bCanEverTick = true;

    /* Space Physics */
    TimeScale             = 1.0f;
    ForceMultiplier       = 1000.0f;
    SunDistanceCm         = 1.5e13;
    SolarPressureAt1AU    = 0.00000456f; 
    AuToUnrealScale       = 10000.0f;
    GravitationalConstant = 6.67430e-11;
    EarthMass             = 5.972e24;

    /* Sail Configuration */
    SailMass              = 5.0f;
    SailArea              = 32.0f;
    GridResolution        = 10;
    InitialSailDistanceKm = 35786.0f;
    bIsDoubleSided        = false;
    bShowPhotonDebug      = false;
    bShowSunDistanceDebug = false;
}

void ASimulationManager::BeginPlay() {
	Super::BeginPlay();
    Instance = this;
    
    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Simulation Manager: ONLINE"));
    }
}

void ASimulationManager::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}
