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
    if (Manager->EnableGravity) {
        SetInitialVelocity();
    } else {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Gravità DISATTIVATA -> Salto configurazione velocità orbitale (Resto fermo)."), *SailName);
    }
    SetInitialRotations();
    if (Manager->EnableCSVLogging) {
        InitializeCSVReporting();
    } else {
        UE_LOG(LogTemp, Log, TEXT("[%s] Logging CSV disabilitato dal Manager."), *SailName);
        CsvFilePath = TEXT("");
        return;
    }
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

void ASolarSail::SetInitialVelocity() {
    if (!IsValid(Manager) || !IsValid(SAIL_MESH)) {
        UE_LOG(LogTemp, Warning, TEXT("[%s] SailMesh o Manager non validi! Impossibile impostare velocità iniziale."), *SailName);
        return;
    }
    FVector3d CurrentPos = FVector3d(GetActorLocation());
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;    
    FVector3d RadialDir = (CurrentPos - EarthPosUU).GetSafeNormal();
    double OrbitRadiusMeters = OrbitRadius * 1000.0;
    InitialOrbitVelocityModule = FMath::Sqrt((Manager->GRAVITATIONAL_CONSTANT * Manager->EARTH_MASS) / OrbitRadiusMeters);
    InitialOrbitVelocityVersor = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();
    if(InitialOrbitVelocityVersor.IsNearlyZero()) {
        InitialOrbitVelocityVersor = FVector3d(1, 0, 0); 
        UE_LOG(LogTemp, Warning, TEXT("[%s] Attenzione: Vela posizionata sopra i poli. Reset versore velocità su asse X."), *SailName);
    }
    InitialOrbitVelocity = InitialOrbitVelocityVersor * InitialOrbitVelocityModule;
    SAIL_MESH->SetPhysicsLinearVelocity(InitialOrbitVelocity);
    UE_LOG(LogTemp, Warning, TEXT("[%s] Velocità orbitale iniziale impostata a %.3f m/s"), *SailName, InitialOrbitVelocityModule);
}

void ASolarSail::InitializeCSVReporting() {
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
    FVector3d GravityAccel = Manager->GetEarthGravityAccelerationAt(SailPosition, SailDistanceFromEarth);
    GravityForce = GravityAccel * SAIL_MASS;
    GravityForceModule = GravityForce.Size();
    GravityForceVersor = GravityForce.GetSafeNormal();
    //FVector UnrealForce = FVector(GravityForce * Manager->KM_TO_UU);
    SAIL_MESH->AddForce(GravityForce);
    UE_LOG(LogTemp, VeryVerbose, TEXT("[%s] Gravità applicata: %.2f N"), *SailName, GravityForceModule);
}
/*
void ASolarSail::UpdateSailRotation(float DeltaTime) {
    if (!Manager || !SAIL_MESH) {
        return;
    }
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    FVector3d RadialDir = (SailPosition - EarthPosUU).GetSafeNormal();
    FVector3d TangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();
    if (TangentDir.IsNearlyZero()) {
        TangentDir = FVector3d(1, 0, 0);
    }
    FRotator TargetRot = FRotationMatrix::MakeFromZ(FVector(TangentDir)).Rotator();
    FRotator CurrentRot = GetActorRotation();
    float InterpSpeed = 2.0f; 
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, InterpSpeed);
    SetActorRotation(NewRot);
    SailNormal = FVector3d(GetActorUpVector());
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();
    CosTheta = FMath::Clamp(FVector3d::DotProduct(SunDirection, SailNormal), 0.0, 1.0);
    IncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(CosTheta));
}
*/

void ASolarSail::UpdateSailRotation(float DeltaTime) {
    if (!Manager || !SAIL_MESH) {
        return;
    }

    // --- 1. GESTIONE MOVIMENTO/ROTAZIONE FISICA (Invariata) ---
    // Questo ruota la mesh nello spazio.
    if (Manager->EnableGravity) {
        FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
        FVector3d RadialDir = (SailPosition - EarthPosUU).GetSafeNormal();
        FVector3d TangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(0, 0, 1)).GetSafeNormal();
        
        if (TangentDir.IsNearlyZero()) {
            TangentDir = FVector3d(1, 0, 0);
        }
        if (FVector3d::DotProduct(TangentDir, SunDirection) < 0) {
            TangentDir = -TangentDir;
        }
        FRotator TargetRot = FRotationMatrix::MakeFromZ(FVector(TangentDir)).Rotator();
        FRotator CurrentRot = GetActorRotation();
        float InterpSpeed = 2.0f; 
        FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, InterpSpeed);
        SetActorRotation(NewRot);
    }

    // --- 2. CALCOLO VETTORI PER LA FISICA (La parte "Smart") ---
    
    // Calcoliamo dove sta il sole rispetto a noi
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();

    // Prendiamo la normale "vera" della mesh
    FVector3d RawNormal = FVector3d(GetActorUpVector());
    
    // Calcoliamo l'allineamento (+1 = fronte al sole, -1 = retro al sole)
    double Alignment = FVector3d::DotProduct(SunDirection, RawNormal);

    // LOGICA DOUBLE SIDED
    if (Alignment < 0) {
        // Il sole colpisce il RETRO
        if (DoubleSidedSail) {
            // Se è doppia faccia: INVERTIAMO la normale virtuale.
            // Così la forza verrà calcolata come se avessimo colpito il fronte.
            SailNormal = -RawNormal; 
            CosTheta = FMath::Abs(Alignment); // Usiamo l'assoluto (diventa positivo)
        } else {
            // Se è singola faccia: Il retro non spinge.
            SailNormal = RawNormal;
            CosTheta = 0.0; // Forza azzerata
        }
    } else {
        // Il sole colpisce il FRONTE (tutto normale)
        SailNormal = RawNormal;
        CosTheta = Alignment; // È già positivo
    }

    // Solo per debug visivo
    IncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosTheta, 0.0, 1.0)));
}
/*
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
    RaycastTimer += DeltaTime;
    if (RaycastTimer < RaycastInterval) {
        if (!SolarForce.ContainsNaN()) {
             SAIL_MESH->AddForce(SolarForce);
        }
        return;
    }
    RaycastTimer = 0.0f;
    ActivePhotons = 0;
    int32 SafeResolution = FMath::Max(1, GridResolution);
    TotalPhotons = SafeResolution * SafeResolution;
    FVector3d AccumulatedForce = FVector3d::Zero();
    double GridSizeUU = 100.0 * SAIL_SCALE; 
    double Step = GridSizeUU / SafeResolution;
    double AreaPerRay = SAIL_AREA / TotalPhotons;
    FVector3d Right = FVector3d(GetActorRightVector());
    FVector3d Forward = FVector3d(GetActorForwardVector());
    for (int32 i = 0; i < SafeResolution; i++) {
        for (int32 j = 0; j < SafeResolution; j++) {
            double OffX = (i - SafeResolution / 2.0) * Step;
            double OffY = (j - SafeResolution / 2.0) * Step;
            FVector3d SamplePos = SailPosition + (Forward * OffX) + (Right * OffY);
            FHitResult Hit;
            FCollisionQueryParams P;
            P.AddIgnoredActor(this);
            FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
            if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(SunPosUU), FVector(SamplePos), ECC_Visibility, P)) {
                ActivePhotons++;
                SolarPressure = Manager->GetSolarPressureAt(SamplePos, SailDistanceFromSun);
                double ForceMag = SelectedReflectivityFactor * SolarPressure * AreaPerRay * (CosTheta * CosTheta) * 2.0;
                AccumulatedForce += (-SailNormal) * ForceMag;
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
*/

void ASolarSail::UpdateSolarForce(float DeltaTime) {
    if (!Manager || !SAIL_MESH) {
        return;
    }

    // 1. Selezione del fattore di riflettività
    double SelectedReflectivityFactor = 0.0;
    switch (Reflectivity_Factor) {
        case EReflectivityPreset::Absorber:  SelectedReflectivityFactor = RFACTOR_ABSORBER;  break;
        case EReflectivityPreset::Medium:    SelectedReflectivityFactor = RFACTOR_MEDIUM;    break;
        case EReflectivityPreset::Realistic: SelectedReflectivityFactor = RFACTOR_REALISTIC; break;
        case EReflectivityPreset::Perfect:   SelectedReflectivityFactor = RFACTOR_PERFECT;   break;
        case EReflectivityPreset::Custom:    SelectedReflectivityFactor = RFACTOR_CUSTOM;    break;
    }

    // 2. Timer per il Raycasting (Ottimizzazione Performance)
    RaycastTimer += DeltaTime;
    if (RaycastTimer < RaycastInterval) {
        // Se non è ancora tempo di ricalcolare, applichiamo la vecchia forza calcolata
        if (!SolarForce.ContainsNaN()) {
             SAIL_MESH->AddForce(SolarForce);
        }
        return;
    }
    // Reset del timer
    RaycastTimer = 0.0f;

    // 3. Setup della Griglia
    ActivePhotons = 0;
    int32 SafeResolution = FMath::Max(1, GridResolution);
    TotalPhotons = SafeResolution * SafeResolution;
    
    FVector3d AccumulatedForce = FVector3d::Zero();
    
    double GridSizeUU = 100.0 * SAIL_SCALE; 
    double Step = GridSizeUU / SafeResolution;
    double AreaPerRay = SAIL_AREA / TotalPhotons;

    FVector3d Right = FVector3d(GetActorRightVector());
    FVector3d Forward = FVector3d(GetActorForwardVector());
    
    // OTTIMIZZAZIONE: Calcoliamo posizione Sole e Parametri Collisione FUORI dal loop
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    
    FCollisionQueryParams P;
    P.AddIgnoredActor(this); // La vela non deve farsi ombra da sola
    P.bTraceComplex = true; // Usa collisioni complesse per maggiore accuratezza

    // Lunghezza visiva delle linee di debug (200 unità = 2 metri)
    float DebugLineLen = 200.0f;

    // 4. Ciclo Raycasting sulla Griglia
    for (int32 i = 0; i < SafeResolution; i++) {
        for (int32 j = 0; j < SafeResolution; j++) {
            
            // Calcolo posizione del punto sulla vela
            double OffX = (i - SafeResolution / 2.0) * Step;
            double OffY = (j - SafeResolution / 2.0) * Step;
            FVector3d SamplePos = SailPosition + (Forward * OffX) + (Right * OffY);
            
            double CheckDistance = 50000.0 * Manager->KM_TO_UU; // 50 Km in Unità Unreal
            
            FVector3d RayStart = SamplePos + (SunDirection * CheckDistance);
            FVector3d RayEnd   = SamplePos;

            FHitResult Hit;

            // Lanciamo il raggio dal Sole verso la Vela
            // Se Hit == true, c'è un ostacolo (Ombra)
           bool bIsOccluded = GetWorld()->LineTraceSingleByChannel(
                Hit, 
                FVector(RayStart), // Start (50km verso il sole)
                FVector(RayEnd),   // End (Sulla vela)
                ECC_Visibility, 
                P
            );

            // --- Calcolo vettori per DEBUG VISIVO (Linee corte) ---
            // Start: Un punto in aria verso il sole
            // End: La superficie della vela
            FVector VisualStart = FVector(SamplePos + (SunDirection * DebugLineLen)); 
            FVector VisualEnd   = FVector(SamplePos);

            if (!bIsOccluded) {
                // --- CASO LUCE (FOTONE ATTIVO) ---
                ActivePhotons++;
                SolarPressure = Manager->GetSolarPressureAt(SamplePos, SailDistanceFromSun);
                
                // Calcolo della forza
                // Nota: CosTheta viene calcolato in UpdateSailRotation ed è sempre >= 0
                double ForceMag = SelectedReflectivityFactor * SolarPressure * AreaPerRay * (CosTheta * CosTheta);
                
                // Applicazione Forza: Opposta alla normale (-SailNormal)
                AccumulatedForce += (-SailNormal) * ForceMag;

                if (Manager->ShowPhotonDebug) {
                    // Disegna linea VERDE corta
                    // LifeTime = RaycastInterval (Niente flashing)
                    DrawDebugLine(GetWorld(), VisualStart, VisualEnd, FColor::Green, false, RaycastInterval, 0, 1.5f);
                }
            } else {
                // --- CASO OMBRA (FOTONE BLOCCATO) ---
                if (Manager->ShowPhotonDebug) {
                    // Disegna linea ROSSA corta (mostra che qui non arriva spinta)
                    DrawDebugLine(GetWorld(), VisualStart, VisualEnd, FColor::Red, false, RaycastInterval, 0, 1.5f);
                }
            }
        }
    }

    // 5. Aggiornamento Variabili Pubbliche e Applicazione Fisica
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

// void ASolarSail::DebugVisuals() {
//     if (!Manager || !GEngine) {
//         return;
//     }
//     FVector SailLoc = GetActorLocation();
//     int32 UID = GetTypeHash(SailName) % 1000; 
//     if (Manager->ShowSailEarthDistanceDebug) {
//         DrawDebugLine(GetWorld(), SailLoc, FVector(Manager->EARTH_POSITION * Manager->KM_TO_UU), FColor::Cyan, false, -1, 0, 2.0f);
//     }
//     if (Manager->ShowSailSunDistanceDebug) {
//         DrawDebugLine(GetWorld(), SailLoc, FVector(Manager->SUN_POSITION * Manager->KM_TO_UU), FColor::Yellow, false, -1, 0, 2.0f);
//     }
//     if (Manager->ShowEarthSunDistanceDebug) {
//         DrawDebugLine(GetWorld(), FVector(Manager->EARTH_POSITION * Manager->KM_TO_UU), FVector(Manager->SUN_POSITION * Manager->KM_TO_UU), FColor::Orange, false, -1, 0, 1.0f);
//     }
//     const float ArrowLen = 1000.0f;
//     if (Manager->ShowSailNormalDebug) {
//         DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(SailNormal) * ArrowLen), 150.f, FColor::Green, false, -1, 0, 5.0f);
//     }
//     if (Manager->ShowSolarForceDebug) {
//         DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(SolarForceVersor) * ArrowLen), 150.f, FColor::Red, false, -1, 0, 5.0f);
//     }
//     if (Manager->ShowGravityForceDebug) {
//         DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(GravityForceVersor) * ArrowLen), 150.f, FColor::Blue, false, -1, 0, 5.0f);
//     }
//     if (Manager->ShowTotalForceDebug) {
//         DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(TotalForce.GetSafeNormal()) * ArrowLen), 200.f, FColor::Magenta, false, -1, 0, 7.0f);
//     }
//     FColor StatusColor = (ActivePhotons > 0) ? FColor::Green : FColor::Red;
//     float Efficiency = (TotalPhotons > 0) ? ((float)ActivePhotons / (float)TotalPhotons) * 100.0f : 0.0f;
//     GEngine->AddOnScreenDebugMessage(UID + 0, 0.0f, FColor::Cyan  , FString::Printf(TEXT("═══ [%s] SIM STATUS ═══"), *SailName));
//     GEngine->AddOnScreenDebugMessage(UID + 1, 0.0f, FColor::White , FString::Printf(TEXT("  Orbita: %.2f Km | Vel: %.3f Km/s"), SailDistanceFromEarth, SailVelocity.Size()));
//     GEngine->AddOnScreenDebugMessage(UID + 2, 0.0f, StatusColor   , FString::Printf(TEXT("  Raggi: %.0f/%d (%.1f%% Attivi)"), (double)ActivePhotons, TotalPhotons, Efficiency));
//     GEngine->AddOnScreenDebugMessage(UID + 3, 0.0f, FColor::Yellow, FString::Printf(TEXT("  Spinta: %.2e N | Angolo: %.1f°"), SolarForceModule, IncidenceAngle));
//     GEngine->AddOnScreenDebugMessage(UID + 4, 0.0f, FColor::Silver, FString::Printf(TEXT("  Massa: %.1f Kg | Area: %.1f m²"), SAIL_MASS, SAIL_AREA));
//     GEngine->AddOnScreenDebugMessage(UID + 5, 0.0f, FColor::Cyan  ,                 TEXT("═══════════════════════"));
// }

void ASolarSail::DebugVisuals() {
    if (!Manager || !GEngine) {
        return;
    }

    FVector SailLoc = GetActorLocation();
    
    // Usiamo un Hash del nome per avere un ID unico per i messaggi a schermo
    // Questo permette di aggiornare il testo invece di spammarne di nuovi
    int32 UID = GetTypeHash(SailName) % 1000; 

    // --- DISEGNO LINEE E FRECCE (Vettori) ---
    // Qui usiamo lifetime -1 (o breve) perché queste linee si aggiornano ad ogni frame del Tick.
    
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

    // --- TELEMETRIA A SCHERMO (UI Testuale) ---

    // FIX FLICKERING: Impostiamo una durata di 1 secondo (1.0f).
    // Se metti 0.0f, il testo dura un solo frame e scompare se il mouse si muove o l'editor lagga.
    float MsgDuration = 1.0f;

    FColor StatusColor = (ActivePhotons > 0) ? FColor::Green : FColor::Red;
    float Efficiency = (TotalPhotons > 0) ? ((float)ActivePhotons / (float)TotalPhotons) * 100.0f : 0.0f;
    
    // Aggiungo info visuale sul tipo di faccia (Singola/Doppia)
    FString SideInfo = DoubleSidedSail ? TEXT("DOPPIA FACCIA") : TEXT("SINGOLA FACCIA");

    GEngine->AddOnScreenDebugMessage(UID + 0, MsgDuration, FColor::Cyan  , FString::Printf(TEXT("═══ [%s] SIM STATUS (%s) ═══"), *SailName, *SideInfo));
    GEngine->AddOnScreenDebugMessage(UID + 1, MsgDuration, FColor::White , FString::Printf(TEXT("  Orbita: %.2f Km | Vel: %.3f Km/s"), SailDistanceFromEarth, SailVelocity.Size()));
    GEngine->AddOnScreenDebugMessage(UID + 2, MsgDuration, StatusColor   , FString::Printf(TEXT("  Raggi: %.0f/%d (%.1f%% Attivi)"), (double)ActivePhotons, TotalPhotons, Efficiency));
    GEngine->AddOnScreenDebugMessage(UID + 3, MsgDuration, FColor::Yellow, FString::Printf(TEXT("  Spinta: %.2e N | Angolo: %.1f°"), SolarForceModule, IncidenceAngle));
    GEngine->AddOnScreenDebugMessage(UID + 4, MsgDuration, FColor::Silver, FString::Printf(TEXT("  Massa: %.1f Kg | Area: %.1f m²"), SAIL_MASS, SAIL_AREA));
    GEngine->AddOnScreenDebugMessage(UID + 5, MsgDuration, FColor::Cyan  ,                 TEXT("══════════════════════════════"));
}

void ASolarSail::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    if (!IsValid(Manager) || !IsValid(SAIL_MESH) || !GEngine) {
        return;
    }
    this->CustomTimeDilation = Manager->TimeScale;
    SailPosition = FVector3d(GetActorLocation());
    SailDistanceFromEarth = FVector3d::Distance(GetActorLocation(), Manager->EARTH_POSITION * Manager->KM_TO_UU) * Manager->UU_TO_KM;
    SailDistanceFromSun = FVector3d::Distance(GetActorLocation(), Manager->SUN_POSITION * Manager->KM_TO_UU) * Manager->UU_TO_KM;
    OrbitRadius = SailDistanceFromEarth;

    UpdateSailRotation(DeltaTime);
    if(Manager->EnableGravity) {
        UpdateGravityForce();
        //AlignSailNormal();
    } else {
        GravityForce = FVector3d::Zero();
        GravityForceModule = 0.0;
        GravityForceVersor = FVector3d::Zero();
    }
    if(Manager->EnableSolarPressure) {
        UpdateSolarForce(DeltaTime);
    } else {
        SolarForce = FVector3d::Zero();
        SolarForceModule = 0.0;
        SolarForceVersor = FVector3d::Zero();
        SolarPressure = 0.0;
        ActivePhotons = 0;
    }
    TotalForce = GravityForce + SolarForce;
    FVector3d AccelMetersS2 = TotalForce / SAIL_MASS;
    SailAcceleration = AccelMetersS2 / 1000.0;

    
    SailVelocity = FVector3d(SAIL_MESH->GetPhysicsLinearVelocity()) * Manager->UU_TO_KM;
    if(Manager->EnableCSVLogging) {
        AppendDataToCSV(DeltaTime);
    }
    if(Manager->ShowDebugTelemetry) {
        DebugVisuals();
    }
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