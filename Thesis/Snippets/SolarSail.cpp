#include "SolarSail.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"

/* COSTRUTTORE
    Funzionamento generale: 
        - Imposta il tick della simulazione su abilitato per permettere l'aggiornamento ad ogni frame
        - Crea un componente StaticMesh per rappresentare la vela e lo imposta come root component
        - Configura le proprietà fisiche della mesh (simulazione fisica, gravità, damping)
        - Inizializza il puntatore al SimulationManager a nullptr

    Quando viene chiamata:
        All'instanziazione dell'attore nella scena, prima di BeginPlay()

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        SAIL_MESH
        Manager
*/
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

/* BEGIN PLAY
    Funzionamento generale: 
        - Inizializza la simulazione collegandosi al SimulationManager
        - Imposta le proprietà fisiche della vela (massa, scala)
        - Configura la posizione, rotazione e velocità iniziale della vela in base al tipo di orbita scelto
        - Inizializza il sistema di reporting su CSV se abilitato

    Quando viene chiamata:
        All'inizio della simulazione, dopo il costruttore

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        - Manager
        - CsvFilePath
*/
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
    if (Manager->EnableGravity) {
        SetInitialPositions();
        SetInitialRotations();
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

/* TERMINA INIZIALIZZAZIONE
    Funzionamento generale: 
        - Funzione di supporto chiamata dopo aver stabilito il collegamento con il SimulationManager
        - Esegue tutte le operazioni di inizializzazione che richiedono l'accesso al Manager, come la configurazione fisica, posizione, rotazione, velocità e reporting CSV

    Quando viene chiamata:
        Dopo aver stabilito con successo il collegamento al SimulationManager, sia al primo tentativo che dopo un retry

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        Nessuna (tutte le modifiche sono delegate alle funzioni chiamate al suo interno)
*/
void ASolarSail::FinishInitialization() {
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
    }

    UE_LOG(LogTemp, Log, TEXT("[%s] Inizializzazione completata con successo!"), *SailName);
}

/* INIZIALIZZAZIONE MANAGER
    Funzionamento generale: 
        - Cerca il SimulationManager nella scena
        - Se non lo trova, tenta di recuperarlo come singleton
        - Se non trova nemmeno il singleton, ritorna false e avvia un retry loop

    Quando viene chiamata:
        All'inizio della simulazione, dopo il costruttore

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        true se il Manager è stato trovato e collegato con successo, false altrimenti

    UPROPERTY modificate: 
        Nessuna (il Manager è un singleton e viene modificato solo qui)
*/
bool ASolarSail::InitializeManager() {
    Manager = ASimulationManager::Instance;
    
    if (!Manager)
    {
        Manager = Cast<ASimulationManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ASimulationManager::StaticClass()));
    }

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
            if (this->InitializeManager()) 
            {
                UE_LOG(LogTemp, Log, TEXT("[%s] Manager trovato al retry! Eseguo FinishInitialization."), *SailName);
                
                this->FinishInitialization(); 
                this->SetActorTickEnabled(true);
            }
        }, 0.2f, false);

        SetActorTickEnabled(false);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[%s] Collegamento al SimulationManager stabilito."), *SailName);
    return true;
}

/* INIZIALIZZAZIONE PROPRIETÀ FISICHE
    Funzionamento generale: 
        - Imposta la massa della vela in base alla costante SAIL_MASS
        - Imposta la scala della vela in base alla costante SAIL_SCALE

    Quando viene chiamata:
        All'inizio della simulazione, dopo aver stabilito il collegamento al SimulationManager

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        SAIL_MESH (massa e scala)
*/
void ASolarSail::InitializePhysicsProperties() {
    if (!SAIL_MESH) {
        UE_LOG(LogTemp, Warning, TEXT("[%s] SailMesh non valida! Impossibile impostare massa e scala."), *SailName);
        return;
    }
    SAIL_MESH->SetMassOverrideInKg(NAME_None, SAIL_MASS, true);
    SetActorScale3D(FVector(SAIL_SCALE));
    UE_LOG(LogTemp, Log, TEXT("[%s] Massa impostata a %.2f Kg, Scala: %.2f"), *SailName, SAIL_MASS, SAIL_SCALE);
}

/* IMPOSTAZIONE POSIZIONI INIZIALI
    Funzionamento generale: 
        - Calcola la posizione iniziale della vela in base al tipo di orbita scelto e alla posizione della terra
        - Aggiorna le variabili interne relative alla posizione e distanza dalla terra

    Quando viene chiamata:
        All'inizio della simulazione, dopo aver stabilito il collegamento al SimulationManager

    Parametri in ingresso: 
        Nessuno (tutte le informazioni necessarie sono ottenute dal Manager o dalle proprietà dell'attore)

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        - Posizione dell'attore (SetActorLocation)
        - SailPosition
        - OrbitRadius
        - SailDistanceFromEarth
*/
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

/* IMPOSTAZIONE ROTAZIONI INIZIALI
    Funzionamento generale: 
        - Allinea l'asse "Blu" (Z) della vela alla direzione tangenziale all'orbita (per farla andare nella direzione desiderata)
        - Mantiene l'orientamento di rollio dato dall'utente (asse "Rosso" o X) proiettandolo sulla tangente
        - Calcola la rotazione finale e la applica all'attore

    Quando viene chiamata:
        All'inizio della simulazione, dopo aver stabilito il collegamento al SimulationManager e impostato la posizione iniziale

    Parametri in ingresso: 
        Nessuno (tutte le informazioni necessarie sono ottenute dal Manager o dalle proprietà dell'attore)

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        Rotazione dell'attore (SetActorRotation)
*/
void ASolarSail::SetInitialRotations() {
    if (!SAIL_MESH || !Manager) return;

    FVector3d CurrentLocation = FVector3d(GetActorLocation());
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    FVector3d RadialDir = (CurrentLocation - EarthPosUU).GetSafeNormal();

    FVector3d UserForward = FVector3d(GetActorUpVector());

    double RadialComponent = FVector3d::DotProduct(UserForward, RadialDir);
    FVector3d ValidTangentDir = UserForward - (RadialDir * RadialComponent);
    
    if (ValidTangentDir.IsNearlyZero()) {
        ValidTangentDir = FVector3d::CrossProduct(RadialDir, FVector3d(1, 0, 0)); // Tangente arbitraria
        if (ValidTangentDir.IsNearlyZero()) ValidTangentDir = FVector3d(0, 1, 0);
    }
    ValidTangentDir.Normalize();
    FVector3d UserUpRef = FVector3d(GetActorForwardVector());
    FRotator NewRotation = FRotationMatrix::MakeFromZX((FVector)ValidTangentDir, (FVector)UserUpRef).Rotator();
    
    SetActorRotation(NewRotation);

    UE_LOG(LogTemp, Log, TEXT("[%s] Rotazione Iniziale: Z(Blu) allineato alla tangente."), *SailName);
}

/* IMPOSTAZIONE VELOCITÀ INIZIALE
    Funzionamento generale: 
        - Calcola la velocità orbitale iniziale necessaria per mantenere l'orbita desiderata in base alla posizione iniziale
        - Applica questa velocità alla mesh della vela

    Quando viene chiamata:
        All'inizio della simulazione, dopo aver stabilito il collegamento al SimulationManager, impostato la posizione e la rotazione iniziale

    Parametri in ingresso: 
        Nessuno (tutte le informazioni necessarie sono ottenute dal Manager o dalle proprietà dell'attore)

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        Velocità lineare della mesh (SetPhysicsLinearVelocity)
        InitialOrbitVelocity
        InitialOrbitVelocityModule
        InitialOrbitVelocityVersor
*/
void ASolarSail::SetInitialVelocity() {
    if (!IsValid(Manager) || !IsValid(SAIL_MESH)) {
        return;
    }

    FVector3d CurrentPos = FVector3d(GetActorLocation());
    FVector3d EarthPosUU = Manager->EARTH_POSITION * Manager->KM_TO_UU;
    double Dist = FVector3d::Distance(CurrentPos, EarthPosUU) * Manager->UU_TO_METERS;
    if (Dist < 1000.0) Dist = 1000.0;
    InitialOrbitVelocityModule = FMath::Sqrt((Manager->GRAVITATIONAL_CONSTANT * Manager->EARTH_MASS) / Dist);

    InitialOrbitVelocityVersor = FVector3d(GetActorUpVector());

    InitialOrbitVelocity = InitialOrbitVelocityVersor * InitialOrbitVelocityModule;
    if(SAIL_MESH->IsSimulatingPhysics()) {
        SAIL_MESH->SetPhysicsLinearVelocity(FVector(InitialOrbitVelocity));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Velocità: %.3f m/s lungo asse Z (Blu)."), *SailName, InitialOrbitVelocityModule);
}

/* INIZIALIZZAZIONE REPORTING CSV
    Funzionamento generale: 
        - Crea la cartella "Csvs" se non esiste
        - Imposta il percorso del file CSV specifico per questa vela
        - Resetta il flag del header per la scrittura

    Quando viene chiamata:
        All'inizio della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        Nessuno (tutte le informazioni necessarie sono ottenute dal Manager o dalle proprietà dell'attore)

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        Nessuno

    Note:
        Il nome del file CSV include il nome dell'attore per differenziare i log in caso di più vele
*/
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

/* AGGIORNAMENTO FORZA DI GRAVITÀ
    Funzionamento generale: 
        - Calcola la forza di gravità attuale sulla vela in base alla posizione e distanza dalla terra
        - Aggiorna le variabili interne relative alla forza di gravità (modulo, versore)
        - Applica la forza di gravità alla mesh della vela

    Quando viene chiamata:
        Ad ogni tick della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        Nessuno (tutte le informazioni necessarie sono ottenute dal Manager o dalle proprietà dell'attore)

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        F = m * a
        a = G * M / r^2 (dove G è la costante gravitazionale, M è la massa della terra e r è la distanza dalla terra)

    Note:
        La forza di gravità viene applicata solo se TimeScale è inferiore o uguale a 1.1 per evitare problemi di stabilità a velocità elevate
*/
void ASolarSail::UpdateGravityForce() {
    if (!SAIL_MESH || !Manager) {
        return;
    }
    FVector3d GravityAccel = Manager->GetEarthGravityAccelerationAt(SailPosition, SailDistanceFromEarth);
    GravityForce = GravityAccel * SAIL_MASS;
    GravityForceModule = GravityForce.Size();
    GravityForceVersor = GravityForce.GetSafeNormal();
    //FVector UnrealForce = FVector(GravityForce * Manager->KM_TO_UU);
    if(Manager->TimeScale <= 1.1f) {
        if(SAIL_MESH->IsSimulatingPhysics()) {
            SAIL_MESH->AddForce(FVector(GravityForce));
        }
    }
}

/* AGGIORNAMENTO ROTAZIONE VELA
    Funzionamento generale: 
        - Aggiorna la rotazione della vela in modo che l'asse "Blu" (Z) sia sempre allineato alla direzione della velocità
        - Se la velocità è molto bassa, mantiene l'orientamento attuale per evitare rotazioni instabili

    Quando viene chiamata:
        Ad ogni tick della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        DeltaTime: Tempo trascorso dall'ultimo tick, usato per interpolare la rotazione in modo fluido

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        Utilizzo di FQuat::FindBetweenNormals per calcolare la rotazione necessaria ad allineare l'asse Z alla direzione della velocità
        Interpolazione con FQuat::Slerp per una transizione morbida

    Note:
        La rotazione viene aggiornata solo se la velocità è sufficientemente alta per evitare problemi di instabilità quando la vela è quasi ferma
*/
void ASolarSail::UpdateSailRotation(float DeltaTime) {
    if (!Manager || !SAIL_MESH) return;

    FVector3d VelocityDir = SailVelocity;
    
    if (VelocityDir.SizeSquared() > 0.001) {
        VelocityDir.Normalize();
        
        FVector CurrentZ = GetActorUpVector(); // La freccia Blu attuale
        FVector TargetVel = (FVector)VelocityDir; 

        double ForwardDot = FVector::DotProduct(CurrentZ, TargetVel);

        FVector TargetOrientation;

        if (ForwardDot > 0) {
            TargetOrientation = TargetVel;
        } 
        else {
            TargetOrientation = -TargetVel;
        }

        FQuat DeltaRot = FQuat::FindBetweenNormals(CurrentZ, TargetOrientation);
        FQuat TargetQuat = DeltaRot * GetActorQuat();

        FQuat NewQuat = FQuat::Slerp(GetActorQuat(), TargetQuat, 2.0f * DeltaTime);
        SetActorRotation(NewQuat);
    }
}

/* AGGIORNAMENTO FORZA SOLARE
    Funzionamento generale: 
        - Calcola la forza solare attuale sulla vela in base alla posizione del sole, all'orientamento della vela e alla riflettività
        - Aggiorna le variabili interne relative alla forza solare (modulo, versore)
        - Applica la forza solare alla mesh della vela

    Quando viene chiamata:
        Ad ogni tick della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        DeltaTime: Tempo trascorso dall'ultimo tick, usato per ottimizzare i calcoli con un timer

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        F = P * A * cos(theta) * R
        Dove:
            P = Pressione solare (costante)
            A = Area della vela
            theta = angolo tra la normale della vela e la direzione del sole
            R = coefficiente di riflettività (da 0 a 1)

    Note:
        La forza solare viene calcolata solo ogni RaycastInterval secondi per ottimizzare le prestazioni, ma viene applicata ad ogni tick per mantenere la continuità.
*/
void ASolarSail::UpdateSolarForce(float DeltaTime) {
    if (!Manager || !SAIL_MESH) return;

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
        if (Manager->TimeScale <= 1.1f && !SolarForce.ContainsNaN()) {
            if(SAIL_MESH->IsSimulatingPhysics()) {
                SAIL_MESH->AddForce(FVector(SolarForce));
            }
        }
        return;
    }
    RaycastTimer = 0.0f;

    LocalActiveRays.Reset();
    LocalBlockedRays.Reset();

    ActivePhotons = 0;
    int32 SafeResolution = FMath::Max(1, GridResolution);
    TotalPhotons = SafeResolution * SafeResolution;
    FVector3d AccumulatedForce = FVector3d::Zero();
    
    SailNormal = FVector3d(GetActorUpVector());
    FVector3d SunPosUU = Manager->SUN_POSITION * Manager->KM_TO_UU;
    SunDirection = (SunPosUU - SailPosition).GetSafeNormal();

    double Alignment = FVector3d::DotProduct(SunDirection, SailNormal);
    
    FVector3d PushDirection;      // La direzione VERSO CUI la vela sarà spinta
    FVector3d RaycastStartOffset; // Offset per non colpire la vela stessa col raggio

    if (Alignment < 0) {
        PushDirection = SailNormal;
        RaycastStartOffset = -SailNormal * 50.0; 
        CosTheta = FMath::Abs(Alignment);
    } 
    else {
        PushDirection = -SailNormal;
        RaycastStartOffset = SailNormal * 50.0;

        if (DoubleSidedSail) {
            CosTheta = Alignment;
        } else {
            CosTheta = 0.0; 
        }
    }

    if (CosTheta <= 0.001) {
        SolarForce = FVector3d::Zero();
        SolarForceModule = 0.0;
        SolarPressure = 0.0;
        return;
    }

    double GridSizeUU = 100.0 * SAIL_SCALE; 
    double Step = GridSizeUU / SafeResolution;
    double AreaPerRay = SAIL_AREA / TotalPhotons;

    FVector3d Right = FVector3d(GetActorRightVector());
    FVector3d Forward = FVector3d(GetActorForwardVector());

    FTransform ActorTrans = GetActorTransform();

    for (int32 i = 0; i < SafeResolution; i++) {
        for (int32 j = 0; j < SafeResolution; j++) {
            
            double OffX = (i - SafeResolution / 2.0 + 0.5) * Step;
            double OffY = (j - SafeResolution / 2.0 + 0.5) * Step;
            FVector3d SamplePos = SailPosition + (Forward * OffX) + (Right * OffY);
            
            FVector3d TraceStart = SamplePos + RaycastStartOffset;
            FVector3d TraceEnd = SunPosUU;

            FHitResult Hit;
            FCollisionQueryParams P;
            P.AddIgnoredActor(this);
            
            bool bHitSomething = GetWorld()->LineTraceSingleByChannel(Hit, FVector(TraceStart), FVector(TraceEnd), ECC_Visibility, P);

            FVector LocalP = ActorTrans.InverseTransformPosition(FVector(SamplePos));

            if (!bHitSomething) {
                ActivePhotons++;
                double LocalPressure = Manager->GetSolarPressureAt(SamplePos, SailDistanceFromSun);
                
                double ForceMag = SelectedReflectivityFactor * LocalPressure * AreaPerRay * (CosTheta * CosTheta) * 2.0;
                
                AccumulatedForce += PushDirection * ForceMag;

                // Debug Verde
                if (Manager->ShowPhotonDebug) {
                    //DrawDebugLine(GetWorld(), FVector(SamplePos), FVector(SamplePos + SunDirection * 200.0), FColor::Green, false, RaycastInterval);
                    LocalActiveRays.Add(LocalP);
                }
            } 
            else if (Manager->ShowPhotonDebug) {
                // Debug Rosso
                //DrawDebugLine(GetWorld(), FVector(SamplePos), FVector(SamplePos + SunDirection * 200.0), FColor::Red, false, RaycastInterval);
                LocalBlockedRays.Add(LocalP);
            }
        }
    }

    SolarForce = AccumulatedForce;
    SolarForceModule = SolarForce.Size();
    SolarForceVersor = SolarForce.GetSafeNormal();

    if (Manager->TimeScale <= 1.1f && !SolarForce.ContainsNaN()) {
        if(SAIL_MESH->IsSimulatingPhysics()) {
            SAIL_MESH->AddForce(FVector(SolarForce));
        }
    }
}

/* AGGIORNAMENTO LOG CSV
    Funzionamento generale: 
        - Se il logging CSV è abilitato, accumula i dati attuali in un buffer
        - Scrive il buffer su file ogni secondo per ottimizzare le prestazioni

    Quando viene chiamata:
        Ad ogni tick della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        DeltaTime: Tempo trascorso dall'ultimo tick, usato per gestire il timer di scrittura

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        Nessuno

    Note:
        Il CSV include dati come tempo, distanza dalla terra e dal sole, velocità, forza totale, pressione solare, angolo di incidenza e numero di fotoni attivi.
*/
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

/* DEBUG VISUALE
    Funzionamento generale: 
        - Se abilitato dal Manager, disegna linee e frecce per visualizzare forze, direzioni, distanze e altri dati utili per il debug
        - Include la visualizzazione dei raggi solari (verdi per attivi, rossi per bloccati) e una scia dell'orbita

    Quando viene chiamata:
        Ad ogni tick della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        Nessuno (tutte le informazioni necessarie sono ottenute dal Manager o dalle proprietà dell'attore)

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        Nessuno

    Note:
        La visualizzazione è completamente configurabile tramite le opzioni del Manager, permettendo di attivare o disattivare specifici elementi a seconda delle esigenze di debug.
*/
void ASolarSail::DebugVisuals() {
    if (!Manager || !GEngine) {
        return;
    }
    FVector SailLoc = GetActorLocation();

    if (Manager->ShowPhotonDebug) {
        FTransform CurrentTrans = GetActorTransform();
        FVector SunDir = (Manager->SUN_POSITION * Manager->KM_TO_UU - SailLoc).GetSafeNormal();
        float RayLen = 200.0f; // Lunghezza visiva

        // Raggi Verdi (Attivi)
        for (const FVector& LP : LocalActiveRays) {
            FVector WorldStart = CurrentTrans.TransformPosition(LP);
            FVector WorldEnd = WorldStart + (SunDir * RayLen);
            DrawDebugLine(GetWorld(), WorldStart, WorldEnd, FColor::Green, false, -1, 0, 1.0f);
        }
        // Raggi Rossi (Bloccati)
        for (const FVector& LP : LocalBlockedRays) {
            FVector WorldStart = CurrentTrans.TransformPosition(LP);
            FVector WorldEnd = WorldStart + (SunDir * RayLen);
            DrawDebugLine(GetWorld(), WorldStart, WorldEnd, FColor::Red, false, -1, 0, 1.0f);
        }
    }
    
    const float ArrowLen = 1000.0f;
    const float ArrowSize = 150.f;

    if (ShowOrbitTrail) {
        bool bAddPoint = false;
        if (OrbitHistory.Num() == 0) {
            bAddPoint = true;
        } else {
            float DistSq = FVector::DistSquared(OrbitHistory.Last(), SailLoc);
            float ThresholdUU = TrailPointMinDistance * Manager->KM_TO_UU;
            
            if (DistSq > (ThresholdUU * ThresholdUU)) {
                bAddPoint = true;
            }
        }

        if (bAddPoint) {
            OrbitHistory.Add(SailLoc);
            // Rimuovi i punti vecchi se superiamo il limite (FIFO)
            if (OrbitHistory.Num() > MaxTrailPoints) {
                OrbitHistory.RemoveAt(0);
            }
        }

        // B. DISEGNO SCIA
        if (OrbitHistory.Num() > 1) {
            for (int32 i = 0; i < OrbitHistory.Num() - 1; i++) {
                float Alpha = (float)i / (float)OrbitHistory.Num();
                FLinearColor ColorLinear = FMath::Lerp(FLinearColor(0.1f, 0.0f, 1.0f, 0.5f), FColor::Cyan, Alpha);
                
                DrawDebugLine(
                    GetWorld(), 
                    OrbitHistory[i], 
                    OrbitHistory[i + 1], 
                    ColorLinear.ToFColor(true), 
                    false, -1, 0, 
                    150.0f // Spessore linea
                );
            }
            DrawDebugLine(GetWorld(), OrbitHistory.Last(), SailLoc, FColor::Cyan, false, -1, 0, 150.0f);
        }
    } else {
        if(OrbitHistory.Num() > 0) OrbitHistory.Empty();
    }

    const float LineThick = 2.0f;

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
    TArray<AActor*> RawSails;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASolarSail::StaticClass(), RawSails);

    TArray<ASolarSail*> ValidSails;
    for (AActor* Actor : RawSails) {
        ASolarSail* S = Cast<ASolarSail>(Actor);
        if (IsValid(S) && S->SailIndex > 0)
        {
            ValidSails.Add(S);
        }
    }

    ValidSails.Sort([](const ASolarSail& A, const ASolarSail& B) {
        if (A.SailIndex == B.SailIndex) {
            return A.GetActorLabel() < B.GetActorLabel();
        }
        return A.SailIndex < B.SailIndex;
    });

    if (ValidSails.Num() == 0 || ValidSails[0] != this) {
        return;
    }

    const int32 LinesPerSail = 16; // Aumentato per coprire tutte le righe + footer
    const int32 RootKey = 50000;   
    const float Dur = 1.0f;

    for (int32 i = 0; i < ValidSails.Num(); ++i) {
        ASolarSail* S = ValidSails[i];
        int32 BaseKey = RootKey - (i * LinesPerSail);

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
        FColor C_Val   = FColor(200, 200, 200);
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

/* TICK PRINCIPALE
    Funzionamento generale: 
        - Gestisce la transizione tra modalità Fisica e Manuale in base al TimeScale del Manager
        - Aggiorna i dati di posizione, distanza, rotazione, forze e velocità
        - Applica le forze alla mesh se in modalità Fisica, altrimenti calcola manualmente il movimento
        - Gestisce l'output dei dati su CSV e debug

    Quando viene chiamata:
        Ad ogni frame della simulazione, dopo aver stabilito il collegamento al SimulationManager e configurato le proprietà iniziali

    Parametri in ingresso: 
        DeltaTime: Tempo trascorso dall'ultimo tick, usato per gestire il tempo di
*/
void ASolarSail::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    
    if (!IsValid(Manager) || !IsValid(SAIL_MESH) || !GEngine) {
        return;
    }

    bool bShouldBePhysics = (Manager->TimeScale <= 1.1f);
    bool bCurrentlyPhysics = SAIL_MESH->IsSimulatingPhysics();

    if (bShouldBePhysics && !bCurrentlyPhysics) {
        SAIL_MESH->SetSimulatePhysics(true);
        SAIL_MESH->SetPhysicsLinearVelocity(FVector(SailVelocity * Manager->KM_TO_UU));
        this->CustomTimeDilation = Manager->TimeScale; // Unreal gestisce il tempo
        UE_LOG(LogTemp, Log, TEXT("[%s] Switch to PHYSICS MODE (TimeScale <= 1.1)"), *SailName);
    }
    else if (!bShouldBePhysics && bCurrentlyPhysics) {
        SailVelocity = FVector3d(SAIL_MESH->GetPhysicsLinearVelocity()) * Manager->UU_TO_KM;
        SAIL_MESH->SetSimulatePhysics(false);
        this->CustomTimeDilation = 1.0f; // Importante: Resettiamo il Dilation a 1, il tempo lo moltiplico io manualmente nei calcoli
        UE_LOG(LogTemp, Log, TEXT("[%s] Switch to MANUAL KINEMATIC MODE (TimeScale > 1.1)"), *SailName);
    }

    SailPosition = FVector3d(GetActorLocation());
    SailDistanceFromEarth = FVector3d::Distance(GetActorLocation(), Manager->EARTH_POSITION * Manager->KM_TO_UU) * Manager->UU_TO_KM;
    SailDistanceFromSun = FVector3d::Distance(GetActorLocation(), Manager->SUN_POSITION * Manager->KM_TO_UU) * Manager->UU_TO_KM;
    OrbitRadius = SailDistanceFromEarth;

    UpdateSailRotation(DeltaTime);

    if(Manager->EnableGravity) {
        UpdateGravityForce();
    } else {
        GravityForce = FVector3d::Zero();
    }

    if(Manager->EnableSolarPressure) {
        UpdateSolarForce(DeltaTime);
    } else {
        SolarForce = FVector3d::Zero();
        SolarPressure = 0.0; // Reset telemetria
        ActivePhotons = 0;
    }

    TotalForce = GravityForce + SolarForce;

    if (bShouldBePhysics) {
        this->CustomTimeDilation = Manager->TimeScale; // Mantengo sincronizzato
        SailVelocity = FVector3d(SAIL_MESH->GetPhysicsLinearVelocity()) * Manager->UU_TO_KM;
        
        SailAcceleration = (TotalForce / SAIL_MASS) / 1000.0;
    } 
    else {

        double SimulationDeltaTime = DeltaTime * Manager->TimeScale;

        // F = m * a  ->  a = F / m
        FVector3d AccelMetersS2 = TotalForce / SAIL_MASS;
        FVector3d AccelKmS2 = AccelMetersS2 / 1000.0;

        // Integrazione Eulero (V = V0 + a*t, P = P0 + v*t)
        SailVelocity += AccelKmS2 * SimulationDeltaTime;
        
        FVector3d DisplacementKm = SailVelocity * SimulationDeltaTime;
        FVector3d DisplacementUU = DisplacementKm * Manager->KM_TO_UU;

        FVector3d NewPosition = SailPosition + DisplacementUU;

        if (!NewPosition.ContainsNaN()) {
            SetActorLocation(FVector(NewPosition));
            SailAcceleration = AccelKmS2; // Per telemetry
        }
    }

    if(Manager->EnableCSVLogging) {
        AppendDataToCSV(DeltaTime);
    }
    if(Manager->ShowDebugTelemetry) {
        DebugVisuals();
    }
}

/* END PLAY
    Funzionamento generale: 
        - Al termine della simulazione, stampa un report finale con i dati più rilevanti (distanza finale, velocità, percorso del CSV)
        - Salva eventuale buffer residuo su disco
        - Logga il motivo della chiusura (distruzione attore, cambio livello, stop in editor, chiusura app)

    Quando viene chiamata:
        Quando l'attore viene distrutto o la simulazione termina (ad esempio, cambio livello o stop in editor)

    Parametri in ingresso: 
        EndPlayReason: Enum che indica il motivo per cui EndPlay è stato chiamato

    Valore di ritorno: 
        Nessuno

     Formule/Riferimenti:
        Nessuno

    Note:
        Il report finale è utile per avere una sintesi dei risultati della simulazione senza dover aprire il CSV, e per verificare che i dati siano stati salvati correttamente.
*/
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