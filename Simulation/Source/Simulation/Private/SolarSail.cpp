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

    //Inizializzazione delle variabili
    SailName = GetName();
    #if WITH_EDITOR
        SailName = GetActorLabel();
    #endif
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Avvio procedura di inizializzazione..."), *SailName);

    // Funzioni di inizializzazione
    if (!InitializeManager()) {
        return;
    }
    InitializePhysicsProperties();
    SetInitialPositions();
    SetInitialRotations();
    if (Manager->EnableGravity) {
        SetInitialVelocity();
    } else {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Gravità DISATTIVATA -> Salto configurazione velocità orbitale (Resto fermo)."), *SailName);
    }
    if (Manager->EnableCSVLogging) {
        InitializeCSVReporting();
    } else {
        UE_LOG(LogTemp, Log, TEXT("[%s] Logging CSV disabilitato dal Manager."), *SailName);
        CsvFilePath = TEXT("");
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("[%s] Inizializzazione completata con successo!"), *SailName);
}

void ASolarSail::FinishInitialization() {
    // A questo punto il Manager esiste SICURO (garantito dal chiamante)
    
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
        // return; // Non serve return qui, siamo alla fine, ma se ci fosse altro codice dopo servirebbe.
    }

    UE_LOG(LogTemp, Log, TEXT("[%s] Inizializzazione completata con successo!"), *SailName);
}

bool ASolarSail::InitializeManager() {
    // Tentativo 1: Singleton
    Manager = ASimulationManager::Instance;
    
    // Tentativo 2: Ricerca diretta (opzionale ma consigliato per robustezza immediata)
    if (!Manager)
    {
        Manager = Cast<ASimulationManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ASimulationManager::StaticClass()));
    }

    // Se ancora nulla, avviamo il retry loop
    if (!Manager) 
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ERRORE CRITICO! SimulationManager non trovato nella scena."), *SailName);
        
        if (GEngine) 
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("[%s] CRITICAL: SimulationManager missing! Retrying..."), *SailName));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[%s] Manager non trovato. Riprovo tra 0.2s..."), *SailName);
        
        FTimerHandle RetryHandle;
        GetWorld()->GetTimerManager().SetTimer(RetryHandle, [this]() 
        {
            // Riprova a chiamare questa stessa funzione
            if (this->InitializeManager()) 
            {
                // Se ha successo, eseguiamo il resto dell'inizializzazione manualmente
                // perché il BeginPlay originale è già terminato da tempo.
                UE_LOG(LogTemp, Log, TEXT("[%s] Manager trovato al retry! Eseguo FinishInitialization."), *SailName);
                
                this->FinishInitialization(); 
                
                // Riabilita il Tick che avevamo spento qui sotto
                this->SetActorTickEnabled(true);
            }
        }, 0.2f, false);

        // Disabilitiamo il tick finché non siamo pronti
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
    if (!SAIL_MESH || !Manager) return;

    FVector3d CurrentLocation = FVector3d(GetActorLocation());
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    FVector3d RadialDir = (CurrentLocation - EarthPosUU).GetSafeNormal();

    // 1. LEGGIAMO LA DIREZIONE "BLU" (Z) DALL'EDITOR
    // Questo è l'asse che tu punti dove vuoi andare.
    FVector3d UserForward = FVector3d(GetActorUpVector());

    // 2. CALCOLIAMO LA TANGENTE BASATA SU Z
    double RadialComponent = FVector3d::DotProduct(UserForward, RadialDir);
    FVector3d ValidTangentDir = UserForward - (RadialDir * RadialComponent);
    
    // Fallback
    if (ValidTangentDir.IsNearlyZero()) {
        ValidTangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(1, 0, 0)); // Tangente arbitraria
        if (ValidTangentDir.IsNearlyZero()) ValidTangentDir = FVector3d(0, 1, 0);
    }
    ValidTangentDir.Normalize();

    // 3. CALCOLIAMO UN NUOVO RIFERIMENTO PER L'ALTO (che ora è X o Y)
    // Se Z è avanti, allora l'asse "Up" del mondo (per non rollare) deve essere mappato su X o Y.
    // Usiamo il Forward Vector attuale (X/Rosso) come vettore "Alto" locale.
    FVector3d UserUpRef = FVector3d(GetActorForwardVector());

    // 4. COSTRUZIONE ROTAZIONE "Z-FORWARD"
    // Z (Up)      = Tangente (La vela va dove punta la freccia Blu)
    // X (Forward) = UserUpRef (Mantiene l'orientamento di rollio che hai dato)
    
    // MakeFromZX: Z è l'asse principale (esatto), X è secondario.
    FRotator NewRotation = FRotationMatrix::MakeFromZX((FVector)ValidTangentDir, (FVector)UserUpRef).Rotator();
    
    SetActorRotation(NewRotation);

    UE_LOG(LogTemp, Log, TEXT("[%s] Rotazione Iniziale: Z(Blu) allineato alla tangente."), *SailName);
}

void ASolarSail::SetInitialVelocity() {
    if (!IsValid(Manager) || !IsValid(SAIL_MESH)) return;

    // 1. Modulo
    FVector3d CurrentPos = FVector3d(GetActorLocation());
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    double Dist = FVector3d::Distance(CurrentPos, EarthPosUU) * Manager->UU_TO_METERS;
    if (Dist < 1000.0) Dist = 1000.0;
    InitialOrbitVelocityModule = FMath::Sqrt((Manager->GRAVITATIONAL_CONSTANT * Manager->EARTH_MASS) / Dist);

    // 2. DIREZIONE
    // IMPORTANTE: Usiamo UpVector (Blu) perché è quello che abbiamo allineato alla tangente.
    InitialOrbitVelocityVersor = FVector3d(GetActorUpVector());

    InitialOrbitVelocity = InitialOrbitVelocityVersor * InitialOrbitVelocityModule;
    SAIL_MESH->SetPhysicsLinearVelocity(FVector(InitialOrbitVelocity));
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Velocità: %.3f m/s lungo asse Z (Blu)."), *SailName, InitialOrbitVelocityModule);
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

void ASolarSail::UpdateSailRotation(float DeltaTime) {
    if (!Manager || !SAIL_MESH) return;

    // --- 1. MANTENIMENTO ASSETTO (Z = Avanti) ---
    // Otteniamo la direzione reale del movimento fisico
    FVector3d VelocityDir = FVector3d(SAIL_MESH->GetPhysicsLinearVelocity());
    
    // Agiamo solo se c'è movimento sufficiente
    if (VelocityDir.SizeSquared() > 0.1) {
        VelocityDir.Normalize();
        
        // CORREZIONE DEL PROBLEMA "AVVITAMENTO":
        // Invece di ricostruire la rotazione da zero (che resetterebbe il tuo rollio),
        // troviamo la rotazione "più breve" per portare l'asse Z attuale (Blu) 
        // a coincidere con la direzione della Velocità.
        
        FVector CurrentZ = GetActorUpVector(); // Il tuo asse "Avanti" attuale
        FVector TargetZ = (FVector)VelocityDir; // Dove deve andare

        // Calcoliamo il Quaterno che rappresenta questa rotazione delta
        FQuat DeltaRot = FQuat::FindBetweenNormals(CurrentZ, TargetZ);

        // Applichiamo la rotazione corrente + il delta
        FQuat TargetQuat = DeltaRot * GetActorQuat();

        // Interpolazione fluida (Slerp) per evitare scatti, ma senza resettare il rollio
        // Aumenta 2.0f se vuoi che sia più reattiva, diminuisci se vuoi più morbidezza
        FQuat NewQuat = FQuat::Slerp(GetActorQuat(), TargetQuat, 2.0f * DeltaTime);
        
        SetActorRotation(NewQuat);
    }

    // --- 2. FISICA SOLARE (Invariata) ---
    SailNormal = FVector3d(GetActorUpVector()); 
    
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();
    
    double Alignment = FVector3d::DotProduct(SunDirection, SailNormal);

    // if (Alignment < 0) {
    //     if (DoubleSidedSail) {
    //         SailNormal = -SailNormal; 
    //         CosTheta = FMath::Abs(Alignment);
    //     } else {
    //         CosTheta = 0.0;
    //     }
    // } else {
    //     CosTheta = Alignment;
    // }

    if (Alignment < 0) {
        // --- COLPO SUL RETRO (CASO STANDARD) ---
        // Questo è il caso in cui la luce spinge la vela "da dietro".
        // Invertiamo la normale perché la forza spinge "in avanti" (verso la freccia Blu)
        SailNormal = -SailNormal; 
        
        // Questo lato funziona SEMPRE (sia Single che Double) perché è il lato riflettente principale
        CosTheta = FMath::Abs(Alignment);
    } 
    else {
        // --- COLPO SUL FRONTE (Lato Struttura) ---
        if (DoubleSidedSail) {
            // Se è doppia faccia, anche il fronte riflette
            CosTheta = Alignment;
        } else {
            // Se è singola faccia, il fronte è INERTE (non genera spinta)
            CosTheta = 0.0;
        }
    }

    IncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosTheta, 0.0, 1.0)));
}

void ASolarSail::UpdateSolarForce(float DeltaTime) {
    if (!Manager || !SAIL_MESH) {
        return;
    }

    // --- SELEZIONE RIFLETTIVITÀ ---
    double SelectedReflectivityFactor = 0.0;
    switch (Reflectivity_Factor) {
        case EReflectivityPreset::Absorber:  SelectedReflectivityFactor = RFACTOR_ABSORBER;  break;
        case EReflectivityPreset::Medium:    SelectedReflectivityFactor = RFACTOR_MEDIUM;    break;
        case EReflectivityPreset::Realistic: SelectedReflectivityFactor = RFACTOR_REALISTIC; break;
        case EReflectivityPreset::Perfect:   SelectedReflectivityFactor = RFACTOR_PERFECT;   break;
        case EReflectivityPreset::Custom:    SelectedReflectivityFactor = RFACTOR_CUSTOM;    break;
    }

    // --- TIMER PER RIDURRE COSTI CPU ---
    RaycastTimer += DeltaTime;
    if (RaycastTimer < RaycastInterval) {
        // Applichiamo forza costante tra un ricalcolo e l'altro
        if (!SolarForce.ContainsNaN()) {
             SAIL_MESH->AddForce(SolarForce);
        }
        return;
    }
    RaycastTimer = 1.0f; // Reset timer

    // --- SETUP GRIGLIA RAYCAST ---
    ActivePhotons = 0;
    int32 SafeResolution = FMath::Max(1, GridResolution);
    TotalPhotons = SafeResolution * SafeResolution;
    
    FVector3d AccumulatedForce = FVector3d::Zero();
    
    // Dimensioni fisiche vela (Assumiamo Plane 100x100 base * Scale)
    double GridSizeUU = 100.0 * SAIL_SCALE; 
    double Step = GridSizeUU / SafeResolution;
    double AreaPerRay = SAIL_AREA / TotalPhotons;

    // Assi locali per muoversi sulla superficie della vela
    // (Assumiamo che la vela sia un Plane standard: X=Avanti, Y=Destra, Z=Normale)
    FVector3d Right = FVector3d(GetActorRightVector());    // Y
    FVector3d Forward = FVector3d(GetActorForwardVector()); // X
    
    // Posizione del Sole in UU
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();

    // Loop sulla griglia
    for (int32 i = 0; i < SafeResolution; i++) {
        for (int32 j = 0; j < SafeResolution; j++) {
            
            // Offset dal centro della vela
            double OffX = (i - SafeResolution / 2.0) * Step + (Step * 0.5); // +0.5 per centrare il raggio nella cella
            double OffY = (j - SafeResolution / 2.0) * Step + (Step * 0.5);
            
            // Punto di partenza sulla superficie della vela
            FVector3d SamplePos = SailPosition + (Forward * OffX) + (Right * OffY);
            
            // --- RAYCAST INVERSO (Dalla Vela -> Verso il Sole) ---
            // È molto più affidabile per rilevare occlusioni vicine.
            // Spostiamo l'inizio leggermente "fuori" dalla superficie (lungo la normale Z) per evitare auto-collisioni
            FVector3d TraceStart = SamplePos + (SailNormal * 10.0); 
            FVector3d TraceEnd = SunPosUU; // Destinazione: Sole

            FHitResult Hit;
            FCollisionQueryParams P;
            P.AddIgnoredActor(this); // Ignora la vela stessa
            
            // Se NON colpiamo nulla tra noi e il sole, allora c'è luce!
            bool bHitSomething = GetWorld()->LineTraceSingleByChannel(Hit, FVector(TraceStart), FVector(TraceEnd), ECC_Visibility, P);

            if (!bHitSomething) {
                // FOTONE ARRIVATO!
                ActivePhotons++;
                
                // Calcolo pressione locale (basata su distanza)
                double LocalPressure = Manager->GetSolarPressureAt(SamplePos, SailDistanceFromSun);
                
                // Formula spinta solare (P * A * (1+R) * cos^2(theta))
                // Nota: Usiamo 2.0 * cos^2 come approssimazione ideale speculare
                double ForceMag = SelectedReflectivityFactor * LocalPressure * AreaPerRay * (CosTheta * CosTheta) * 2.0;
                
                // Direzione spinta: Sempre opposta alla normale (spinge "dentro" la vela)
                AccumulatedForce += (-SailNormal) * ForceMag;

                // Debug Verde (Luce arriva)
                if (Manager->ShowPhotonDebug) {
                    FVector LineEnd = FVector(SamplePos + SunDirection * 100.0);
                    DrawDebugLine(GetWorld(), FVector(SamplePos), LineEnd, FColor::Green, false, 0.1f);
                }
            } 
            else {
                // FOTONE BLOCCATO (Ombra)
                if (Manager->ShowPhotonDebug) {
                    // Disegna linea rossa fino all'ostacolo
                    FVector LineEnd = FVector(SamplePos + SunDirection * 100.0);
                    DrawDebugLine(GetWorld(), FVector(SamplePos), LineEnd, FColor::Red, false, 0.1f);
                }
            }
        }
    }

    // --- AGGIORNAMENTO FORZE ---
    SolarForce = AccumulatedForce;
    SolarForceModule = SolarForce.Size();
    SolarForceVersor = SolarForce.GetSafeNormal();

    // Applica forza accumulata
    if (!SolarForce.ContainsNaN()) {
        SAIL_MESH->AddForce(SolarForce);
    }
}


// void ASolarSail::UpdateSolarForce(float DeltaTime) {
//     if (!Manager || !SAIL_MESH) {
//         return;
//     }
//     double SelectedReflectivityFactor = 0.0;
//     switch (Reflectivity_Factor) {
//         case EReflectivityPreset::Absorber:  SelectedReflectivityFactor = RFACTOR_ABSORBER;  break;
//         case EReflectivityPreset::Medium:    SelectedReflectivityFactor = RFACTOR_MEDIUM;    break;
//         case EReflectivityPreset::Realistic: SelectedReflectivityFactor = RFACTOR_REALISTIC; break;
//         case EReflectivityPreset::Perfect:   SelectedReflectivityFactor = RFACTOR_PERFECT;   break;
//         case EReflectivityPreset::Custom:    SelectedReflectivityFactor = RFACTOR_CUSTOM;    break;
//     }
//     RaycastTimer += DeltaTime;
//     if (RaycastTimer < RaycastInterval) {
//         if (!SolarForce.ContainsNaN()) {
//              SAIL_MESH->AddForce(SolarForce);
//         }
//         return;
//     }
//     RaycastTimer = 0.0f;
//     ActivePhotons = 0;
//     int32 SafeResolution = FMath::Max(1, GridResolution);
//     TotalPhotons = SafeResolution * SafeResolution;
//     FVector3d AccumulatedForce = FVector3d::Zero();
//     double GridSizeUU = 100.0 * SAIL_SCALE; 
//     double Step = GridSizeUU / SafeResolution;
//     double AreaPerRay = SAIL_AREA / TotalPhotons;
//     FVector3d Right = FVector3d(GetActorRightVector());
//     FVector3d Forward = FVector3d(GetActorForwardVector());
//     for (int32 i = 0; i < SafeResolution; i++) {
//         for (int32 j = 0; j < SafeResolution; j++) {
//             double OffX = (i - SafeResolution / 2.0) * Step;
//             double OffY = (j - SafeResolution / 2.0) * Step;
//             FVector3d SamplePos = SailPosition + (Forward * OffX) + (Right * OffY);
//             FHitResult Hit;
//             FCollisionQueryParams P;
//             P.AddIgnoredActor(this);
//             FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
//             if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(SunPosUU), FVector(SamplePos), ECC_Visibility, P)) {
//                 ActivePhotons++;
//                 SolarPressure = Manager->GetSolarPressureAt(SamplePos, SailDistanceFromSun);
//                 double ForceMag = SelectedReflectivityFactor * SolarPressure * AreaPerRay * (CosTheta * CosTheta) * 2.0;
//                 AccumulatedForce += (-SailNormal) * ForceMag;
//                 if (Manager->ShowPhotonDebug) {
//                     DrawDebugLine(GetWorld(), FVector(SamplePos), FVector(SamplePos - SunDirection * 200.0), FColor::Green, false, 0.05f, 0, 0.5f);
//                 }
//             } else if (Manager->ShowPhotonDebug) {
//                 DrawDebugLine(GetWorld(), Hit.Location, FVector(SamplePos), FColor::Red, false, 0.05f, 0, 0.5f);
//             }
//         }
//     }
//     SolarForce = AccumulatedForce;
//     SolarForceModule = SolarForce.Size();
//     SolarForceVersor = SolarForce.GetSafeNormal();
//     SAIL_MESH->AddForce(SolarForce);
// }

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
    const float ArrowLen = 1000.0f;
    const float LineThick = 2.0f;
    const float ArrowSize = 150.f;

    if (Manager->ShowSailEarthDistanceDebug) DrawDebugLine            (GetWorld(), SailLoc, Manager->EARTH_POSITION * Manager->KM_TO_UU, FColor::Cyan, false, -1, 0, LineThick);
    if (Manager->ShowSailSunDistanceDebug)   DrawDebugLine            (GetWorld(), SailLoc, Manager->SUN_POSITION * Manager->KM_TO_UU, FColor::Yellow, false, -1, 0, LineThick);
    if (Manager->ShowEarthSunDistanceDebug)  DrawDebugLine            (GetWorld(), Manager->EARTH_POSITION * Manager->KM_TO_UU, Manager->SUN_POSITION * Manager->KM_TO_UU, FColor::Orange, false, -1, 0, 1.0f);
    if (Manager->ShowSailNormalDebug)        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(SailNormal) * ArrowLen), ArrowSize, FColor::Green, false, -1, 0, 5.0f);
    if (Manager->ShowSolarForceDebug)        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(SolarForceVersor) * ArrowLen), ArrowSize, FColor::Red, false, -1, 0, 5.0f);
    if (Manager->ShowGravityForceDebug)      DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(GravityForceVersor) * ArrowLen), ArrowSize, FColor::Blue, false, -1, 0, 5.0f);
    if (Manager->ShowTotalForceDebug)        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (FVector(TotalForce.GetSafeNormal()) * ArrowLen), 200.f, FColor::Magenta, false, -1, 0, 7.0f);
    if (Manager->ShowOrientationDebug) {
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (GetActorForwardVector() * ArrowLen), ArrowSize, FColor::Blue, false, -1, 0, 5.0f);
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (GetActorUpVector() * ArrowLen), ArrowSize, FColor::Green, false, -1, 0, 5.0f);
        DrawDebugDirectionalArrow(GetWorld(), SailLoc, SailLoc + (GetActorRightVector() * ArrowLen), ArrowSize, FColor::Red, false, -1, 0, 5.0f);
    }
    if(!Manager->ShowDebugTelemetry) {
        return;
    }
    // A. Troviamo tutte le vele
    TArray<AActor*> RawSails;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASolarSail::StaticClass(), RawSails);

    TArray<ASolarSail*> ValidSails;
    for (AActor* Actor : RawSails)
    {
        ASolarSail* S = Cast<ASolarSail>(Actor);
        // Filtriamo: deve essere valido E avere un Index > 0
        if (IsValid(S) && S->SailIndex > 0)
        {
            ValidSails.Add(S);
        }
    }

    // B. Ordiniamo la lista (Crescente per Index, poi Alfabetico)
    ValidSails.Sort([](const ASolarSail& A, const ASolarSail& B) {
        if (A.SailIndex == B.SailIndex) {
            return A.GetActorLabel() < B.GetActorLabel();
        }
        return A.SailIndex < B.SailIndex;
    });

    // C. Controllo "Master": Solo la prima vela stampa per tutti
    if (ValidSails.Num() == 0 || ValidSails[0] != this)
    {
        return;
    }

    // D. Loop di stampa
    const int32 LinesPerSail = 16; // Aumentato per coprire tutte le righe + footer
    const int32 RootKey = 50000;   
    const float Dur = 1.0f;

    for (int32 i = 0; i < ValidSails.Num(); ++i) {
        ASolarSail* S = ValidSails[i];
        int32 BaseKey = RootKey - (i * LinesPerSail);

        // --- CALCOLI ---
        float SpeedKms = S->SailVelocity.Length();
        float ForceN = S->SolarForceModule;
        float EfficiencyVal = (S->TotalPhotons > 0) ? ((float)S->ActivePhotons / (float)S->TotalPhotons) : 0.0f;

        // Barra Grafica (10 segmenti)
        int32 BarSegments = 10;
        int32 FilledSegments = (int32)(EfficiencyVal * BarSegments);
        FString AsciiBar = TEXT("");
        for (int32 b = 0; b < BarSegments; b++) {
            AsciiBar += (b < FilledSegments) ? TEXT("■") : TEXT("□");
        }
        FString StatusStr = (S->ActivePhotons > 0) ? TEXT("ACTIVE") : TEXT("SHADOW");

        FColor C_Frame = FColor::Cyan;
        FColor C_Text  = FColor::White;
        FColor C_Val   = FColor(200, 200, 200); // Grigio chiaro per i valori
        FColor C_Warn  = (EfficiencyVal > 0) ? FColor::Green : FColor::Red;
        
        //Stringhe di debug
        FString R00 = FString::Printf(TEXT("╔═[ID %d]══[%s]════════════"), (int32)S->SailIndex, *S->GetActorLabel());
        FString R01 = FString::Printf(TEXT("║ Sail Mass: %.1f kg"), S->SAIL_MASS);
        FString R02 = FString::Printf(TEXT("║ Sail Area: %.1f m²"), S->SAIL_AREA);
        FString R03 = FString::Printf(TEXT("╠════════════════════════════")); 
        FString R04 = FString::Printf(TEXT("║ Orbit Radius: %.1f Km"), S->SailDistanceFromEarth);
        FString R05 = FString::Printf(TEXT("║ Velocity: %.3f Km/s"), SpeedKms);
        FString R06 = FString::Printf(TEXT("╠════════════════════════════"));
        FString R07 = FString::Printf(TEXT("║ Incidence Angle: %.1f Deg"), S->IncidenceAngle);
        FString R08 = FString::Printf(TEXT("║ Solar Force: %.5f N"), ForceN);
        FString R09 = FString::Printf(TEXT("║ Active Photons: %d / %d"), S->ActivePhotons, S->TotalPhotons);
        FString R10 = FString::Printf(TEXT("║ Efficiency: [%s] %s"), *AsciiBar, *StatusStr);
        FString R11 = FString::Printf(TEXT("╚════════════════════════════"));
        FString R12 = FString::Printf(TEXT(" "));

        //Aggiunta a schermo
        GEngine->AddOnScreenDebugMessage(BaseKey - 0, Dur, C_Frame, R00);
        GEngine->AddOnScreenDebugMessage(BaseKey - 1, Dur, C_Val,   R01);
        GEngine->AddOnScreenDebugMessage(BaseKey - 2, Dur, C_Val,   R02);
        GEngine->AddOnScreenDebugMessage(BaseKey - 3, Dur, C_Frame, R03);
        GEngine->AddOnScreenDebugMessage(BaseKey - 4, Dur, C_Val,   R04);
        GEngine->AddOnScreenDebugMessage(BaseKey - 5, Dur, C_Val,   R05);
        GEngine->AddOnScreenDebugMessage(BaseKey - 6, Dur, C_Frame, R06);
        GEngine->AddOnScreenDebugMessage(BaseKey - 7, Dur, C_Val,   R07);
        GEngine->AddOnScreenDebugMessage(BaseKey - 8, Dur, C_Val,   R08);
        GEngine->AddOnScreenDebugMessage(BaseKey - 9, Dur, C_Warn,  R09);
        GEngine->AddOnScreenDebugMessage(BaseKey - 10,Dur, C_Warn,  R10);
        GEngine->AddOnScreenDebugMessage(BaseKey - 11,Dur, C_Frame, R11);
        GEngine->AddOnScreenDebugMessage(BaseKey - 12,Dur, C_Frame, R12);
    }
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