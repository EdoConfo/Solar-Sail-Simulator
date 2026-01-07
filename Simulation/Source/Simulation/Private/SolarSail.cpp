// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSail.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"

ASolarSail::ASolarSail()
{
    PrimaryActorTick.bCanEverTick = true; // Abilita il Tick
    SailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SailMesh")); // Mesh della vela solare
    RootComponent = SailMesh;

    // Setup fisica spaziale della vela solare 
    SailMesh->SetSimulatePhysics(true); // Attivazione fisica
    SailMesh->SetEnableGravity(false); // Disattivazione gravità
    SailMesh->SetLinearDamping(0.0f); // Disattivazione attrito lineare
    SailMesh->SetAngularDamping(0.0f); // Disattivazione attrito angolare

    // Valori iniziali
    SailArea = 32.0f; // Area della vela in m^2 (default 32 m^2)
    TotalMass = 5.0f; // Massa iniziale (default 5kg)
    SailMesh->SetMassOverrideInKg(NAME_None, TotalMass, true); // Sovrascrittura massa in kg usando la variabile
    SolarPressureAt1AU = 0.00000456f; // Costante fisica in N/m^2 (4.56 µN/m^2)
    // TODO - DA TOGLIERE DOPO AVER CAPITO COME MODIFICARE IL TEMPO DI SIMULAZIONE IN UNREAL
    ForceMultiplier = 100000.0f; // Moltiplicatore forza per debug/test (Default: 1.0)
    AuToUnrealScale = 10000.0f; // 1 AU = 10.000 cm (100 m) (TODO forse da aumentare per più precisione)
}

void ASolarSail::BeginPlay()
{
    Super::BeginPlay();
    
    // Aggiorno la massa fisica con quella impostata nell'Editor
    if (SailMesh)
    {
        SailMesh->SetMassOverrideInKg(NAME_None, TotalMass, true);
    }

    FindSunInScene(); // Trova il Sole nella scena
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Inizializzazione Vela Solare: Completata!")); // Messaggio di debug all'inizio
    }
}

void ASolarSail::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);    
    UpdateSolarForce(DeltaTime); // Calcola e applica la pressione solare
}

void ASolarSail::FindSunInScene()
{
    if (SunLightActor) // Se già trovato, esci
    {
        return;
    }
    TArray<AActor*> FoundActors; // Lista temporanea per salvare gli attori trovati
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), FoundActors); // Trova tutti gli attori di classe DirectionalLight

    if (FoundActors.Num() > 0) // Se ne ha trovato almeno uno
    {
        SunLightActor = Cast<ADirectionalLight>(FoundActors[0]); // Prendo il primo (assumo sia il Sole)
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ERRORE: Manca il Sole (DirectionalLight)!")); // Messaggio di errore se non trova il Sole
    }
}

void ASolarSail::UpdateSolarForce(float DeltaTime)
{
    if (!SunLightActor || !SailMesh) // Controllo se manca il Sole o la Mesh
    {
        return;
    }

    FVector SailPosition = GetActorLocation(); // Posizione della vela
    FVector SunPosition = SunLightActor->GetActorLocation(); // Posizione del Sole

    FVector LightDirection = (SailPosition - SunPosition).GetSafeNormal(); // Direzione della luce solare (dal Sole alla Vela)
    FVector SailNormal = GetActorUpVector(); // Normale della vela (vettore perpendicolare alla superficie)

    FHitResult HitResult; // Serve per salvare il risultato del trace
    FCollisionQueryParams QueryParams; // Serve per configurare il trace
    QueryParams.AddIgnoredActor(this); // Senza questo la vela si fa ombra da sola (odio profondo)

    bool bHit = GetWorld()->LineTraceSingleByChannel( // Traccia un raggio per verificare l'ombra
        HitResult, // Dove salvare il risultato
        SunPosition, // Punto di partenza (Sole)
        SailPosition, // Punto di arrivo (Vela)
        ECC_Visibility, // Canale di collisione
        QueryParams // Parametri di query (senza vela)
    );

    if (bHit && HitResult.GetActor() != this) // Se il raggio ha colpito qualcosa e non è la vela stessa (non c'è contatto diretto con il Sole)
    {
        CurrentSolarForce = FVector::ZeroVector; // Forza zero
        DrawDebugLine(GetWorld(), SailPosition, SunPosition, FColor::Silver, false, -1.0f, 0, 1.0f); // Linea grigia per indicare l'ombra
        return; 
    }

    float Distance = FVector::Dist(SunPosition, SailPosition); // Distanza in cm tra Sole e Vela
    float DistanceInAU = Distance / AuToUnrealScale; // Conversione in Unità Astronomiche
    CurrentDistanceAU = DistanceInAU; // Salvo per visualizzarlo nell'editor
    float SafeDistance = FMath::Max(DistanceInAU, 0.001f); // Serve per evitare divisione per zero
    float LocalPressure = SolarPressureAt1AU / (SafeDistance * SafeDistance); // Pressione solare locale (legge dell'inverso del quadrato)
    float CosTheta = FVector::DotProduct(-LightDirection, SailNormal); // Angolo di incidenza (coseno tra direzione luce e normale vela)
    
    if (CosTheta < 0) // Se l'angolo è maggiore di 90 gradi
    {
        CosTheta = -CosTheta; // Prendo valore positivo
        SailNormal = -SailNormal; // La forza spingerà dall'altra parte
    }

    CurrentIncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(CosTheta)); // Salvo angolo in gradi per visualizzarlo nell'editor
    float ForceMagnitude = 2.0f * LocalPressure * SailArea * (CosTheta * CosTheta); // Magnitudo della forza solare (N)
    // TODO - DA TOGLIERE DOPO AVER CAPITO COME MODIFICARE IL TEMPO DI SIMULAZIONE IN UNREAL
    ForceMagnitude *= ForceMultiplier; // Applico il moltiplicatore di forza per debug/test
    FVector ForceVector = -SailNormal * ForceMagnitude; // Vettore forza solare (direzione opposta alla normale della vela)
    FVector UnrealForce = ForceVector * 100.0f; // Conversione da Newton a unità di forza di Unreal (1 N = 100 kg·cm/s²)

    if (SailMesh->IsSimulatingPhysics()) // Applico la forza solo se la fisica è attiva
    {
        SailMesh->WakeRigidBody(); // Risveglia il corpo rigido se è in stato di sonno
        SailMesh->AddForce(UnrealForce); // Applica la forza alla vela
    }
    
    CurrentSolarForce = ForceVector; // Salvo la forza attuale per visualizzarla nell'editor

    DrawDebugLine(GetWorld(), SunPosition, SailPosition, FColor::Yellow, false, -1.0f, 0, 2.0f); // Linea gialla per indicare la luce solare
    DrawDebugDirectionalArrow(GetWorld(), SailPosition, SailPosition + (ForceVector.GetSafeNormal() * 500.0f * CosTheta), 50.0f, FColor::Red, false, -1.0f, 0, 5.0f); // Freccia rossa per indicare la forza solare applicata

    if (GEngine) // Messaggio di debug
    {
        FString DebugMsg = FString::Printf(TEXT("Ang: %.1f deg | F: %.2e N | Dist: %.2f AU"), // Formato del messaggio
            CurrentIncidenceAngle, // Angolo in gradi
            CurrentSolarForce.Size(), // Magnitudo forza in Newton
            CurrentDistanceAU); // Distanza in AU
        GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::White, DebugMsg);
    }
}