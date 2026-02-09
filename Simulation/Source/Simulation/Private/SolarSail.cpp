// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSail.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"

ASolarSail::ASolarSail() {
    PrimaryActorTick.bCanEverTick = true;
    SAIL_MESH = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SailMesh"));
    RootComponent = SAIL_MESH;
    SAIL_MESH->SetSimulatePhysics(true);
    SAIL_MESH->SetEnableGravity(false);
    SAIL_MESH->SetLinearDamping(0.0f);
    SAIL_MESH->SetAngularDamping(0.0f);
    Manager = nullptr;
}

void ASolarSail::BeginPlay() {
    Super::BeginPlay();
    SailName = GetName();
    #if WITH_EDITOR
        SailName = GetActorLabel();
    #endif
    UE_LOG(LogTemp, Log, TEXT("[%s] Avvio procedura di inizializzazione..."), *SailName);
    if (!InitializeManager()) {
        return;
    }
    InitializePhysicsProperties();
    SetInitialPositions();
    SetInitialRotations();
    InitializeCSVReporting();
    UE_LOG(LogTemp, Log, TEXT("[%s] Inizializzazione completata con successo!"), *SailName);
}

bool ASolarSail::InitializeManager() {
    Manager = ASimulationManager::Instance;
    if (!Manager) {
        UE_LOG(LogTemp, Error, TEXT("[%s] ERRORE CRITICO! SimulationManager non trovato nella scena."), *SailName);
        if (GEngine) {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("[%s] CRITICAL: SimulationManager missing!"), *SailName));
        }
        SetActorTickEnabled(false);
        return false;
    }
    UE_LOG(LogTemp, Log, TEXT("[%s] Collegamento al SimulationManager stabilito."), *SailName);
    return true;
}

void ASolarSail::InitializePhysicsProperties() {
    if (!SAIL_MESH) {
        UE_LOG(LogTemp, Warning, TEXT("[%s] SailMesh non valida! Impossibile impostare massa e scala."), *SailName);
        return;
    }
    SAIL_MESH->SetMassOverrideInKg(NAME_None, SAIL_MASS, true);
    SetActorScale3D(FVector(SAIL_SCALE));
    UE_LOG(LogTemp, Log, TEXT("[%s] Massa impostata a %.2f Kg, Scala: %.2f"), *SailName, SAIL_MASS, SAIL_SCALE);
}

void ASolarSail::SetInitialPositions() {
    if (!SAIL_MESH) {
        UE_LOG(LogTemp, Warning, TEXT("[%s] SailMesh non valida! Impossibile impostare posizione e velocità iniziali."), *SailName);
        return;
    }
    FVector3d EditorLocation = FVector3d(GetActorLocation()); 
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    FVector3d RadialDir = (EditorLocation - EarthPosUU).GetSafeNormal();
    if (RadialDir.IsNearlyZero()) {
        RadialDir = FVector3d(1, 0, 0); 
        UE_LOG(LogTemp, Warning, TEXT("[%s] Attenzione: Vela posizionata nel centro della Terra. Reset su asse X."), *SailName);
    }
    double SelectedAltitudeKm = 0.0;
    switch (OrbitType) {
        case EOrbitStartType::LEO_ISS:        SelectedAltitudeKm = ORBIT_LEO;    break;
        case EOrbitStartType::MEO_GPS:        SelectedAltitudeKm = ORBIT_GPS;    break;
        case EOrbitStartType::Geostationary:  SelectedAltitudeKm = ORBIT_GEO;    break;
        case EOrbitStartType::LunarDistance:  SelectedAltitudeKm = ORBIT_MOON;   break;
        case EOrbitStartType::CustomAltitude: SelectedAltitudeKm = ORBIT_CUSTOM; break;
    }
    double TargetOrbitRadiusKm = SelectedAltitudeKm + Manager->EARTH_RADIUS;
    double TargetOrbitRadiusUU = TargetOrbitRadiusKm * Manager->KM_TO_UU;
    FVector3d NewLocationUU = EarthPosUU + (RadialDir * TargetOrbitRadiusUU);
    SetActorLocation(FVector(NewLocationUU));
    SailPosition = NewLocationUU; // Aggiorniamo la variabile interna
    OrbitRadius = TargetOrbitRadiusKm;
    SailDistanceFromEarth = OrbitRadius;
    double OrbitRadiusMeters = OrbitRadius * 1000.0;
    InitialOrbitVelocityModule = FMath::Sqrt((Manager->GRAVITATIONAL_CONSTANT * Manager->EARTH_MASS) / OrbitRadiusMeters);
    InitialOrbitVelocityVersor = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();
    if(InitialOrbitVelocityVersor.IsNearlyZero()) {
        InitialOrbitVelocityVersor = FVector3d(1, 0, 0); 
        UE_LOG(LogTemp, Warning, TEXT("[%s] Attenzione: Vela posizionata sopra i poli. Reset versore velocità su asse X."), *SailName);
    }
    InitialOrbitVelocity = InitialOrbitVelocityVersor * InitialOrbitVelocityModule;
    SAIL_MESH->SetPhysicsLinearVelocity(InitialOrbitVelocity);
    UE_LOG(LogTemp, Warning, TEXT("[%s] Posizionata a %.2f Km. Velocità orbitale richiesta: %.3f m/s"), *SailName, OrbitRadius, InitialOrbitVelocityModule);
}

void ASolarSail::SetInitialRotations() {
    if (!SAIL_MESH) {
        UE_LOG(LogTemp, Warning, TEXT("[%s] SailMesh non valida! Impossibile impostare rotazione iniziale."), *SailName);
        return;
    }
    FVector3d CurrentLocation = FVector3d(GetActorLocation());
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    FVector3d RadialDir = (CurrentLocation - EarthPosUU).GetSafeNormal();
    FVector3d TangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();
    if (TangentDir.IsNearlyZero()) {
        TangentDir = FVector3d(1, 0, 0);
        UE_LOG(LogTemp, Warning, TEXT("[%s] Attenzione: Vela posizionata sopra i poli. Reset versore tangente su asse X."), *SailName);
    }
    FRotator InitialRot = FRotationMatrix::MakeFromZ(FVector(TangentDir)).Rotator();
    SetActorRotation(InitialRot);
    UE_LOG(LogTemp, Warning, TEXT("[%s] Rotazione iniziale impostata. Normale vela allineata alla tangente orbitale."), *SailName);
}

void ASolarSail::InitializeCSVReporting() {
    if (!Manager || !Manager->EnableCSVLogging) {
        UE_LOG(LogTemp, Log, TEXT("[%s] Logging CSV disabilitato dal Manager."), *SailName);
        CsvFilePath = TEXT("");
        return;
    }
    FString ProjectDir = FPaths::ProjectDir();
    FString CsvsDir = FPaths::Combine(ProjectDir, TEXT("Csvs"));
    if (!IFileManager::Get().MakeDirectory(*CsvsDir, true)) {
        UE_LOG(LogTemp, Error, TEXT("[%s] Impossibile creare la cartella Csvs!"), *SailName);
        return;
    }
    FString FileName = FString::Printf(TEXT("SolarSailData_%s.csv"), *SailName);
    CsvFilePath = FPaths::Combine(CsvsDir, FileName);
    bCsvHeaderWritten = false;
    UE_LOG(LogTemp, Warning, TEXT("[%s] Log pronto in -> %s"), *SailName, *CsvFilePath);
}

void ASolarSail::UpdateGravityForce() {
    if (!SAIL_MESH || !Manager) {
        return;
    }
    if (!Manager->EnableGravity) {
        GravityForce = FVector3d::Zero();
        GravityForceModule = 0.0;
        GravityForceVersor = FVector3d::Zero();
        return;
    }
    FVector3d GravityAccel = Manager->GetEarthGravityAccelerationAt(SailPosition, SailDistanceFromEarth);
    GravityForce = GravityAccel * SAIL_MASS;
    GravityForceModule = GravityForce.Size();
    GravityForceVersor = GravityForce.GetSafeNormal();
    //FVector UnrealForce = FVector(GravityForce * Manager->KM_TO_UU);
    SAIL_MESH->AddForce(GravityForce);
    UE_LOG(LogTemp, VeryVerbose, TEXT("[%s] Gravità applicata: %.2f N"), *SailName, GravityForceModule);
}

void ASolarSail::UpdateSailRotation(float DeltaTime) {
    if (!Manager || !SAIL_MESH) {
        return;
    }

    // --- 1. Calcolo dei vettori geometrici ---
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    
    // Vettore che va dalla Terra alla Vela (Raggio)
    FVector3d RadialDir = (SailPosition - EarthPosUU).GetSafeNormal();

    // Vettore Tangente (Direzione della Velocità Orbitale)
    // Lo calcoliamo matematicamente come perpendicolare al raggio e all'asse Z del mondo
    FVector3d TangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();

    // Correzione per orbite polari (se siamo esattamente sopra il polo)
    if (TangentDir.IsNearlyZero()) {
        TangentDir = FVector3d(1, 0, 0);
    }

    // --- 2. Definizione della Rotazione Target ---
    // Vogliamo che la NORMALE (Asse Z/Blu della mesh) guardi lungo la TANGENTE.
    // Usiamo MakeFromZ per dire a Unreal: "L'asse Z deve puntare in questa direzione".
    FRotator TargetRot = FRotationMatrix::MakeFromZ(FVector(TangentDir)).Rotator();

    // --- 3. Interpolazione Fluida (RInterpTo) ---
    FRotator CurrentRot = GetActorRotation();
    
    // Velocità di rotazione: puoi usare una variabile come RotationSpeed o un valore fisso (es. 2.0f)
    float InterpSpeed = 2.0f; 
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, InterpSpeed);
    
    SetActorRotation(NewRot);

    // --- 4. Aggiornamento Variabili Fisiche ---
    // Ora la normale è l'UpVector, che sta puntando lungo la tangente
    SailNormal = FVector3d(GetActorUpVector());
    
    // Ricalcoliamo l'angolo rispetto al Sole per la fisica
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();
    
    CosTheta = FMath::Clamp(FVector3d::DotProduct(SunDirection, SailNormal), 0.0, 1.0);
    IncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(CosTheta));
}

/*
void ASolarSail::AlignSailNormal() {
    if (!Manager) {
        return;
    }
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    FVector3d RadialDir = (SailPosition - EarthPosUU).GetSafeNormal();
    FVector3d TangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();
    FRotator NewRot = FRotationMatrix::MakeFromXZ(FVector(TangentDir), FVector(RadialDir)).Rotator();
    SetActorRotation(NewRot);
    SailNormal = FVector3d(GetActorUpVector());
    CosTheta = FMath::Clamp(FVector3d::DotProduct(SunDirection, SailNormal), 0.0, 1.0);
    IncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(CosTheta));
}
*/

void ASolarSail::UpdateSolarForce(float DeltaTime) {
    if (!Manager || !SAIL_MESH) {
        return;
    }
    double SelectedReflectivityFactor = 0.0;
    switch (Reflectivity_Factor) {
        case EReflectivityPreset::Absorber:  SelectedReflectivityFactor = RFACTOR_ABSORBER;  break;
        case EReflectivityPreset::Medium:    SelectedReflectivityFactor = RFACTOR_MEDIUM;    break;
        case EReflectivityPreset::Realistic: SelectedReflectivityFactor = RFACTOR_REALISTIC; break;
        case EReflectivityPreset::Perfect:   SelectedReflectivityFactor = RFACTOR_PERFECT;   break;
        case EReflectivityPreset::Custom:    SelectedReflectivityFactor = RFACTOR_CUSTOM;    break;
    }
    if (!Manager->EnableSolarPressure) {
        SolarForce = FVector3d::Zero();
        SolarForceModule = 0.0;
        SolarForceVersor = FVector3d::Zero();
        SolarPressure = 0.0;
        ActivePhotons = 0;
        return;
    }
    RaycastTimer += DeltaTime;
    if (RaycastTimer < RaycastInterval) {
        SAIL_MESH->AddForce(SolarForce);
        return;
    }
    RaycastTimer = 0.0f;
    ActivePhotons = 0;
    TotalPhotons = Manager->GridResolution * Manager->GridResolution;
    FVector3d AccumulatedForce = FVector3d::Zero();
    double GridSizeUU = 100.0 * SAIL_SCALE; 
    double Step = GridSizeUU / Manager->GridResolution;
    double AreaPerRay = SAIL_AREA / TotalPhotons;
    FVector3d Right = FVector3d(GetActorRightVector());
    FVector3d Forward = FVector3d(GetActorForwardVector());
    for (int32 i = 0; i < Manager->GridResolution; i++) {
        for (int32 j = 0; j < Manager->GridResolution; j++) {
            double OffX = (i - Manager->GridResolution / 2.0) * Step;
            double OffY = (j - Manager->GridResolution / 2.0) * Step;
            FVector3d SamplePos = SailPosition + (Forward * OffX) + (Right * OffY);
            FHitResult Hit;
            FCollisionQueryParams P;
            P.AddIgnoredActor(this);
            FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
            if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(SunPosUU), FVector(SamplePos), ECC_Visibility, P)) {
                ActivePhotons++;
                SolarPressure = Manager->GetSolarPressureAt(SamplePos, SailDistanceFromSun);
                double ForceMag = SelectedReflectivityFactor * SolarPressure * AreaPerRay * (CosTheta * CosTheta) * 2.0;
                AccumulatedForce += SailNormal * ForceMag;
                if (Manager->ShowPhotonDebug) {
                    DrawDebugLine(GetWorld(), FVector(SamplePos), FVector(SamplePos - SunDirection * 200.0), FColor::Green, false, 0.05f, 0, 0.5f);
                }
            } else if (Manager->ShowPhotonDebug) {
                DrawDebugLine(GetWorld(), Hit.Location, FVector(SamplePos), FColor::Red, false, 0.05f, 0, 0.5f);
            }
        }
    }
    SolarForce = AccumulatedForce;
    SolarForceModule = SolarForce.Size();
    SolarForceVersor = SolarForce.GetSafeNormal();
    SAIL_MESH->AddForce(SolarForce);
}

void ASolarSail::AppendDataToCSV(float DeltaTime) {
    if (CsvFilePath.IsEmpty()) {
        return;
    }
    if (!bCsvHeaderWritten) {
        FString Header = TEXT("SailName,Timestamp(s),DistEarth(Km),DistSun(Km),Velocity(Km/s),TotalForce(N),SolarPressure(Pa),IncidenceAngle(deg),ActivePhotons\n");
        if (FFileHelper::SaveStringToFile(Header, *CsvFilePath)) {
            bCsvHeaderWritten = true;
            UE_LOG(LogTemp, Log, TEXT("%s: Header CSV scritto con successo."), *SailName);
        }
    }
    FString DataRow = FString::Printf(TEXT("%s,%.3f,%.2f,%.4f,%.4f,%.4e,%.4e,%.2f,%d\n"), *SailName, GetWorld()->GetTimeSeconds(), SailDistanceFromEarth, SailDistanceFromSun / 149597870.7, SailVelocity.Size(), TotalForce.Size(), SolarPressure, IncidenceAngle, ActivePhotons);
    CsvBuffer += DataRow;
    CsvWriteTimer += DeltaTime;
    if (CsvWriteTimer > 1.0f) {
        FFileHelper::SaveStringToFile(CsvBuffer, *CsvFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
        CsvBuffer.Empty();
        UE_LOG(LogTemp, Log, TEXT("%s: Dati CSV scritti."), *SailName);
        CsvWriteTimer = 0.0f;
    }
}

void ASolarSail::DebugVisuals() {
    if (!Manager || !GEngine) {
        return;
    }
    FVector SailLoc = GetActorLocation();
    int32 UID = GetTypeHash(SailName) % 1000; 
    if (Manager->ShowSailEarthDistanceDebug) {
        DrawDebugLine(GetWorld(), SailLoc, FVector(Manager->EARTH_POSITION * Manager->KM_TO_UU), FColor::Cyan, false, -1, 0, 2.0f);
    }
    if (Manager->ShowSailSunDistanceDebug) {
        DrawDebugLine(GetWorld(), SailLoc, FVector(Manager->SUN_POSITION * Manager->KM_TO_UU), FColor::Yellow, false, -1, 0, 2.0f);
    }
    if (Manager->ShowEarthSunDistanceDebug) {
        DrawDebugLine(GetWorld(), FVector(Manager->EARTH_POSITION * Manager->KM_TO_UU), FVector(Manager->SUN_POSITION * Manager->KM_TO_UU), FColor::Orange, false, -1, 0, 1.0f);
    }
    const float ArrowLen = 1000.0f;
    if (Manager->ShowSailNormalDebug) {
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(SailNormal) * ArrowLen), 150.f, FColor::Green, false, -1, 0, 5.0f);
    }
    if (Manager->ShowSolarForceDebug) {
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(SolarForceVersor) * ArrowLen), 150.f, FColor::Red, false, -1, 0, 5.0f);
    }
    if (Manager->ShowGravityForceDebug) {
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(GravityForceVersor) * ArrowLen), 150.f, FColor::Blue, false, -1, 0, 5.0f);
    }
    if (Manager->ShowTotalForceDebug) {
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(TotalForce.GetSafeNormal()) * ArrowLen), 200.f, FColor::Magenta, false, -1, 0, 7.0f);
    }
    FColor StatusColor = (ActivePhotons > 0) ? FColor::Green : FColor::Red;
    float Efficiency = (TotalPhotons > 0) ? ((float)ActivePhotons / (float)TotalPhotons) * 100.0f : 0.0f;
    GEngine->AddOnScreenDebugMessage(UID + 0, 0.0f, FColor::Cyan, FString::Printf(TEXT("═══ [%s] SIM STATUS ═══"), *SailName));
    GEngine->AddOnScreenDebugMessage(UID + 1, 0.0f, FColor::White, FString::Printf(TEXT("  Orbita: %.2f Km | Vel: %.3f Km/s"), SailDistanceFromEarth, SailVelocity.Size()));
    GEngine->AddOnScreenDebugMessage(UID + 2, 0.0f, StatusColor,   FString::Printf(TEXT("  Raggi: %.0f/%d (%.1f%% Attivi)"), (double)ActivePhotons, TotalPhotons, Efficiency));
    GEngine->AddOnScreenDebugMessage(UID + 3, 0.0f, FColor::Yellow, FString::Printf(TEXT("  Spinta: %.2e N | Angolo: %.1f°"), SolarForceModule, IncidenceAngle));
    GEngine->AddOnScreenDebugMessage(UID + 4, 0.0f, FColor::Silver, FString::Printf(TEXT("  Massa: %.1f Kg | Area: %.1f m²"), SAIL_MASS, SAIL_AREA));
    GEngine->AddOnScreenDebugMessage(UID + 5, 0.0f, FColor::Cyan,  TEXT("═══════════════════════"));
}

void ASolarSail::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    if (!Manager || !SAIL_MESH || !GEngine) {
        return;
    }
    this->CustomTimeDilation = Manager->TimeScale;
    SailPosition = FVector3d(GetActorLocation());
    SailDistanceFromEarth = FVector3d::Distance(GetActorLocation(), Manager->EARTH_POSITION * Manager->KM_TO_UU) * Manager->UU_TO_KM;
    SailDistanceFromSun = FVector3d::Distance(GetActorLocation(), Manager->SUN_POSITION * Manager->KM_TO_UU) * Manager->UU_TO_KM;
    OrbitRadius = SailDistanceFromEarth;
    UpdateGravityForce();
    UpdateSailRotation(DeltaTime);
    //AlignSailNormal();
    UpdateSolarForce(DeltaTime);
    TotalForce = GravityForce + SolarForce;
    FVector3d AccelMetersS2 = TotalForce / SAIL_MASS;
    SailAcceleration = AccelMetersS2 / 1000.0; 
    SailVelocity = FVector3d(SAIL_MESH->GetPhysicsLinearVelocity()) * Manager->UU_TO_KM;
    AppendDataToCSV(DeltaTime);
    DebugVisuals();
}

void ASolarSail::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    UE_LOG(LogTemp, Warning, TEXT("[%s] Fine Simulazione rilevata."), *SailName);
    UE_LOG(LogTemp, Log, TEXT("[%s] --- REPORT FINALE MISSIONE ---"), *SailName);
    UE_LOG(LogTemp, Log, TEXT("[%s] Distanza Finale dalla Terra: %.2f Km"), *SailName, SailDistanceFromEarth);
    UE_LOG(LogTemp, Log, TEXT("[%s] Velocità Finale: %.3f Km/s"), *SailName, SailVelocity.Size());
    UE_LOG(LogTemp, Log, TEXT("[%s] Dati salvati in: %s"), *SailName, *CsvFilePath);
    UE_LOG(LogTemp, Log, TEXT("[%s] ---------------------------------"), *SailName);
    if (!CsvBuffer.IsEmpty() && !CsvFilePath.IsEmpty()) {
        FFileHelper::SaveStringToFile(CsvBuffer, *CsvFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
        UE_LOG(LogTemp, Warning, TEXT("[%s] Buffer CSV residuo salvato su disco."), *SailName);
    }
    FString ReasonStr;
    switch (EndPlayReason) {
        case EEndPlayReason::Destroyed: ReasonStr = TEXT("Attore Distrutto"); break;
        case EEndPlayReason::LevelTransition: ReasonStr = TEXT("Cambio Livello"); break;
        case EEndPlayReason::EndPlayInEditor: ReasonStr = TEXT("Sessione Terminata (STOP)"); break;
        case EEndPlayReason::Quit: ReasonStr = TEXT("Chiusura Applicazione"); break;
        default: ReasonStr = TEXT("Sconosciuto"); break;
    }
    UE_LOG(LogTemp, Log, TEXT("[%s] Motivo chiusura: %s"), *SailName, *ReasonStr);
    Super::EndPlay(EndPlayReason);
}