 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "SolarSail.generated.h"

UCLASS()
class SIMULATION_API ASolarSail : public AActor
{
	GENERATED_BODY() // Classe per simulare una vela solare
	
public:
	ASolarSail(); // Costruttore

protected:
	virtual void BeginPlay() override; // Creazione iniziale

public:
	virtual void Tick(float DeltaTime) override; // Si ripete ogni frame

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Solar Sail") // Mesh della vela solare
	UStaticMeshComponent* SailMesh;

	// Valori Fisici

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Data") // Area in m^2 della vela (default 32 m^2)
	float SailArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Data") // Massa totale in kg della vela (default 5 kg)
	float TotalMass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Data") // Costante fisica (default 4.56 µN/m^2)
    float SolarPressureAt1AU;

    // TODO - DA TOGLIERE DOPO AVER CAPITO COME MODIFICARE IL TEMPO DI SIMULAZIONE IN UNREAL
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Data") // Moltiplicatore forza per debug/test (Default: 1.0)
	float ForceMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Data") // Convertitore Unità Astronomiche (AU) -> Unità Unreal (cm)
    float AuToUnrealScale;

    // Dati a schermo

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry") // Forza solare attuale applicata alla vela
    FVector CurrentSolarForce;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry") // Angolo di incidenza attuale della luce solare sulla vela
    float CurrentIncidenceAngle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry") // Distanza attuale dal Sole in Unità Astronomiche
    float CurrentDistanceAU;

    // Riferimenti Scena

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System") // Sole
    ADirectionalLight* SunLightActor;

private:
    void FindSunInScene(); // Trova il Sole nella scena
    void UpdateSolarForce(float DeltaTime); // Calcola e applica la pressione solare
};