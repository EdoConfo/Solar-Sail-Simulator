#include "SimulationManager.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"

// Inizializzazione del puntatore statico a nullptr per sicurezza
ASimulationManager* ASimulationManager::Instance = nullptr;

/* COSTRUTTORE
    Funzionamento generale: 
        - Imposta il tick della simulazione su abilitato per permettere l'aggiornamento ad ogni frame

    Quando viene chiamata:
        All'instanziazione dell'attore nella scena, prima di BeginPlay()

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        Nessuna
*/
ASimulationManager::ASimulationManager() {
    PrimaryActorTick.bCanEverTick = true;
}

/* BEGIN PLAY
    Funzionamento generale: 
        - Imposta l'istanza statica del singleton a this
        - Pulisce la cartella "Csvs" eliminando i file csv al suo interno
        - Controlla che le mesh della simulazione siano assegnate, altrimenti blocca la simulazione
        - Imposta la posizione e scala della mesh della terra
        - Imposta la posizione e scala della mesh del sole, disabilitando le collisioni
        - Imposta la posizione e rotazione della luce del sole, puntandola verso la terra

    Quando viene chiamata:
        All'inizio della simulazione, dopo il costruttore

    Parametri in ingresso: 
        Nessuno

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        - Instance (singleton)
        - EARTH_MESH_ACTOR
        - SUN_MESH_ACTOR
        - SUN_LIGHT_ACTOR
*/
void ASimulationManager::BeginPlay() {
    // Da lasciare in questo ordine per evitare problemi di Race Condition con il singleton statico
    Instance = this;
    Super::BeginPlay();

    // Pulizia della cartella "Csvs" all'avvio della simulazione
    // Invece di eliminare l'intera cartella elimino solo i file csv al suo interno
    FString CsvDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Csvs"));
    // IFileManager è un'interfaccia di Unreal Engine per la gestione dei file
    if (IFileManager::Get().DirectoryExists(*CsvDir)) {
        TArray<FString> FoundFiles;
        IFileManager::Get().FindFiles(FoundFiles, *CsvDir, TEXT(".csv"));
        for (FString File : FoundFiles) {
            FString FullPath = FPaths::Combine(CsvDir, File);
            IFileManager::Get().Delete(*FullPath);
        }
        UE_LOG(LogTemp, Warning, TEXT("[SimulationManager] Cartella 'Csvs' ripulita (%d file rimossi)."), FoundFiles.Num());
    }

    // Controllo che le mesh della simulazione siano assegnate, altrimenti blocco la simulazione
    checkf(EARTH_MESH_ACTOR, TEXT("[SimulationManager] EARTH_MESH_ACTOR non assegnato nel pannello Details!"));
    checkf(SUN_MESH_ACTOR,   TEXT("[SimulationManager] SUN_MESH_ACTOR non assegnato nel pannello Details!"));
    checkf(SUN_LIGHT_ACTOR,  TEXT("[SimulationManager] SUN_LIGHT_ACTOR non assegnato nel pannello Details!"));

    EARTH_MESH_ACTOR->SetActorLocation(EARTH_POSITION * KM_TO_UU);
    EARTH_MESH_ACTOR->SetActorScale3D(FVector(EARTH_SCALE));
    UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Posizione Terra impostata a %s UU, Scala impostata a %f."), *(EARTH_MESH_ACTOR->GetActorLocation()).ToString(), EARTH_SCALE);

    SUN_MESH_ACTOR->SetActorLocation(SUN_POSITION * KM_TO_UU);
    SUN_MESH_ACTOR->SetActorScale3D(FVector(SUN_SCALE));
    if (UPrimitiveComponent* MeshComp = Cast<UPrimitiveComponent>(SUN_MESH_ACTOR->GetRootComponent())) {
        // Disabilito le collisioni della mesh del sole perché la Directional Light è al suo interno
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Posizione Sole impostata a %s UU, Scala impostata a %f."), *(SUN_MESH_ACTOR->GetActorLocation()).ToString(), SUN_SCALE);

    SUN_LIGHT_ACTOR->SetActorLocation(SUN_POSITION * KM_TO_UU);
    // La Directional Light è sempre ruotata verso la terra
    FVector LookAtVector = (EARTH_MESH_ACTOR->GetActorLocation() - SUN_LIGHT_ACTOR->GetActorLocation()).GetSafeNormal();
    SUN_LIGHT_ACTOR->SetActorRotation(LookAtVector.Rotation());
    UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Posizione Sole impostata a %s UU, Luce puntata verso %s."), *(SUN_LIGHT_ACTOR->GetActorLocation()).ToString(), *LookAtVector.ToString());
}

/* TICK
    Funzionamento generale: 
        - Aggiorna ad ogni frame la rotazione della luce del sole per mantenerla sempre puntata verso la terra
        - Se abilitato, disegna una linea di debug che rappresenta la distanza tra terra e sole

    Quando viene chiamata:
        Ad ogni frame della simulazione

    Parametri in ingresso: 
        - DeltaTime: Tempo trascorso dall'ultimo frame, usato per scalare il tempo della simulazione

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        Nessuna
*/
void ASimulationManager::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    // Doppia verifica che le mesh della simulazione siano assegnate, altrimenti blocco la simulazione
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

/* END PLAY
    Funzionamento generale: 
        - Pulisce l'istanza statica del singleton se è uguale a this
        - Logga un messaggio di fine simulazione

    Quando viene chiamata:
        Alla fine della simulazione

    Parametri in ingresso: 
        - EndPlayReason: Motivo per cui è terminata la simulazione

    Valore di ritorno: 
        Nessuno

    UPROPERTY modificate: 
        Instance (singleton)
*/
void ASimulationManager::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if(Instance == this) {
        Instance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[SimulationManager] Istanza statica pulita con successo."));
    }
    Super::EndPlay(EndPlayReason);
}

/* CALCOLO ACCELERAZIONE GRAVITAZIONALE
    Funzionamento generale: 
        - Calcola l'accelerazione gravitazionale della vela in un punto specifico dello spazio
        - Calcola la direzione verso il quale deve essere applicata la forza di gravità

    Quando viene chiamata:
        Ad ogni Tick dalla classe SolarSail.cpp

    Parametri in ingresso: 
        - SailLocationUU: Posizione spaziale dell'oggetto in Unreal Units
        - SailDistanceFromEarthKm: Distanza pre-calcolata dalla classe chiamante

    Valore di ritorno: 
        Vettore FVector3d che rappresenta l'accelerazione gravitazionale direzionata verso la terra

    UPROPERTY modificate: 
        Nessuna

    Formule/Riferimenti:
        Legge di Gravitazione Universale semplificata per l'accelerazione: a = (G * M) / r^2

    Note:
        I parametri in ingresso sono in Km e convertiti in m per il calcolo (G usa i metri)
*/
FVector3d ASimulationManager::GetEarthGravityAccelerationAt(FVector3d SailLocationUU, double SailDistanceFromEarthKm) const {
    double DistanceFromEarthKm = SailDistanceFromEarthKm > 0.0 ? SailDistanceFromEarthKm : FVector3d::Distance(SailLocationUU, EARTH_POSITION * KM_TO_UU) * UU_TO_KM;
    double DistanceFromEarthM = DistanceFromEarthKm * 1000.0;
    double GravityAccelerationMagnitude = (GRAVITATIONAL_CONSTANT * EARTH_MASS) / (DistanceFromEarthM * DistanceFromEarthM);
    FVector3d DirectionToEarth = (EARTH_POSITION - (SailLocationUU * UU_TO_KM)).GetSafeNormal();
    return DirectionToEarth * GravityAccelerationMagnitude;
}

/* CALCOLO PRESSIONE SOLARE
    Funzionamento generale: 
        - Calcola la pressione solare su un punto specifico dello spazio
        - La forza di pressione solare sarà poi calcolata moltiplicando questa pressione per l'area della vela

    Quando viene chiamata:
        Ad ogni Tick dalla classe SolarSail.cpp

    Parametri in ingresso: 
        - SailLocationUU: Posizione spaziale dell'oggetto in Unreal Units
        - SailDistanceFromSunKm: Distanza pre-calcolata dalla classe chiamante

    Valore di ritorno: 
        Valore double che rappresenta la pressione solare

    UPROPERTY modificate: 
        Nessuna

    Formule/Riferimenti:
        Pressione Solare: P = L / (4 * π * r^2 * c)
        Dove:
            L = Luminosità del Sole (W)
            r = Distanza dal Sole (m)
            c = Velocità della Luce (m/s)

    Note:
        I parametri in ingresso sono in Km e convertiti in m per il calcolo (L usa i metri)
*/
double ASimulationManager::GetSolarPressureAt(FVector3d SailLocationUU, double SailDistanceFromSunKm) const {
    double DistanceFromSunKm = SailDistanceFromSunKm > 0.0 ? SailDistanceFromSunKm : FVector3d::Distance(SailLocationUU, SUN_POSITION * KM_TO_UU) * UU_TO_KM;
    double DistanceFromSunM = DistanceFromSunKm * 1000.0;
    double SolarPressure = SOLAR_LUMINOSITY / (4.0 * PI_GREEK * DistanceFromSunM * DistanceFromSunM * (SPEED_OF_LIGHT * 1000.0));
    return SolarPressure * ForceMultiplier;
}