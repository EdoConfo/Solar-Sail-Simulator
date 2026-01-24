#include "SimulationManager.h"

ASimulationManager* ASimulationManager::Instance = nullptr;

ASimulationManager::ASimulationManager() {
	PrimaryActorTick.bCanEverTick = true;
    SolarPressureAt1AU = 0.00000456f; 
    AuToUnrealScale = 10000.0f;
    TimeScale = 1.0f;
    ForceMultiplier = 1000.0f; 
    SailMass = 5.0f;
    SailArea = 32.0f;
    GridResolution = 10;
    bShowPhotonDebug = false;
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
