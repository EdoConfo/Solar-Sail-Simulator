#include "SimulationManager.h"

ASimulationManager* ASimulationManager::Instance = nullptr;

ASimulationManager::ASimulationManager() {
    PrimaryActorTick.bCanEverTick = true;
}

void ASimulationManager::BeginPlay() {
    Instance = this;
    Super::BeginPlay();
    FString CsvDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Csvs"));
    if (IFileManager::Get().DirectoryExists(*CsvDir)) {
        TArray<FString> FoundFiles;
        IFileManager::Get().FindFiles(FoundFiles, *CsvDir, TEXT(".csv"));
        
        for (FString File : FoundFiles) {
            FString FullPath = FPaths::Combine(CsvDir, File);
            IFileManager::Get().Delete(*FullPath);
        }
        UE_LOG(LogTemp, Warning, TEXT("[SimulationManager] Cartella 'Csvs' ripulita (%d file rimossi)."), FoundFiles.Num());
    }

    checkf(EARTH_MESH_ACTOR, TEXT("[SimulationManager] EARTH_MESH_ACTOR non assegnato nel pannello Details!"));
    checkf(SUN_MESH_ACTOR,   TEXT("[SimulationManager] SUN_MESH_ACTOR non assegnato nel pannello Details!"));
    checkf(SUN_LIGHT_ACTOR,  TEXT("[SimulationManager] SUN_LIGHT_ACTOR non assegnato nel pannello Details!"));

    EARTH_MESH_ACTOR->SetActorLocation(EARTH_POSITION * KM_TO_UU);
    EARTH_MESH_ACTOR->SetActorScale3D(FVector(EARTH_SCALE));
    UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Posizione Terra impostata a %s UU, Scala impostata a %f."), *(EARTH_MESH_ACTOR->GetActorLocation()).ToString(), EARTH_SCALE);

    SUN_MESH_ACTOR->SetActorLocation(SUN_POSITION * KM_TO_UU);
    SUN_MESH_ACTOR->SetActorScale3D(FVector(SUN_SCALE));
    if (UPrimitiveComponent* MeshComp = Cast<UPrimitiveComponent>(SUN_MESH_ACTOR->GetRootComponent())) {
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Posizione Sole impostata a %s UU, Scala impostata a %f."), *(SUN_MESH_ACTOR->GetActorLocation()).ToString(), SUN_SCALE);

    SUN_LIGHT_ACTOR->SetActorLocation(SUN_POSITION * KM_TO_UU);
    FVector LookAtVector = (EARTH_MESH_ACTOR->GetActorLocation() - SUN_LIGHT_ACTOR->GetActorLocation()).GetSafeNormal();
    SUN_LIGHT_ACTOR->SetActorRotation(LookAtVector.Rotation());
    UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Posizione Sole impostata a %s UU, Luce puntata verso %s."), *(SUN_LIGHT_ACTOR->GetActorLocation()).ToString(), *LookAtVector.ToString());
}

void ASimulationManager::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    if (!EARTH_MESH_ACTOR || !SUN_MESH_ACTOR || !SUN_LIGHT_ACTOR) {
        return;
    }

    double ScaledDeltaTime = DeltaTime * TimeScale;

    if (ShowEarthSunDistanceDebug) {
        DrawDebugLine(GetWorld(), EARTH_MESH_ACTOR->GetActorLocation(), SUN_MESH_ACTOR->GetActorLocation(), FColor::Yellow, false, -1, 0, 5.0f );
    }
    
    FVector LookAtVector = (EARTH_MESH_ACTOR->GetActorLocation() - SUN_LIGHT_ACTOR->GetActorLocation()).GetSafeNormal();
    SUN_LIGHT_ACTOR->SetActorRotation(LookAtVector.Rotation());
}

void ASimulationManager::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if(Instance == this) {
        Instance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Istanza statica pulita con successo."));
    }
    Super::EndPlay(EndPlayReason);
}

FVector3d ASimulationManager::GetEarthGravityAccelerationAt(FVector3d SailLocationUU, double SailDistanceFromEarthKm) const {
    double DistanceFromEarthKm = SailDistanceFromEarthKm > 0.0 ? SailDistanceFromEarthKm : FVector3d::Distance(SailLocationUU, EARTH_POSITION * KM_TO_UU) * UU_TO_KM;
    double DistanceFromEarthM = DistanceFromEarthKm * 1000.0;
    double GravityAccelerationMagnitude = (GRAVITATIONAL_CONSTANT * EARTH_MASS) / (DistanceFromEarthM * DistanceFromEarthM);
    FVector3d DirectionToEarth = (EARTH_POSITION - (SailLocationUU * UU_TO_KM)).GetSafeNormal();
    return DirectionToEarth * GravityAccelerationMagnitude * ForceMultiplier;
}

double ASimulationManager::GetSolarPressureAt(FVector3d SailLocationUU, double SailDistanceFromSunKm) const {
    double DistanceFromSunKm = SailDistanceFromSunKm > 0.0 ? SailDistanceFromSunKm : FVector3d::Distance(SailLocationUU, SUN_POSITION * KM_TO_UU) * UU_TO_KM;
    double DistanceFromSunM = DistanceFromSunKm * 1000.0;
    double SolarPressure = SOLAR_LUMINOSITY / (4.0 * PI_GREEK * DistanceFromSunM * DistanceFromSunM * (SPEED_OF_LIGHT * 1000.0));
    return SolarPressure * ForceMultiplier;
}