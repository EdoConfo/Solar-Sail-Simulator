// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimulationManager.h"
#include "SolarSail.generated.h"

UENUM(BlueprintType)
enum class EOrbitStartType : uint8 {
    LEO_ISS             UMETA(DisplayName = "LEO (Stazione Spaziale - 400km)"),
    MEO_GPS             UMETA(DisplayName = "MEO (GPS - 20.200km)"),
    Geostationary       UMETA(DisplayName = "GEO (Geostazionaria - 35.786km)"),
    LunarDistance       UMETA(DisplayName = "Distanza Lunare (384.400km)"),
    CustomAltitude      UMETA(DisplayName = "Altitudine Personalizzata")
};

UCLASS(HideCategories = (Rendering, Replication, Input, Actor, LOD, Cooking, Collision, HLOD, DataLayers, Networking))
class SIMULATION_API ASolarSail : public AActor {
    GENERATED_BODY()

public:
    ASolarSail();
    virtual void Tick(float DeltaTime) override;
    
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail"                 , meta = (DisplayName = "Mesh della Vela"                                                                                                                               )) TObjectPtr<UStaticMeshComponent> SAIL_MESH                  = nullptr;
    UPROPERTY(EditAnywhere   , Category = "Bodies | Sail"                 , meta = (DisplayName = "Area della Vela (m^2)"                                                                              , ClampMin = "9.3"  , ClampMax = "1672.0"  )) double                           SAIL_AREA                  = 100.0;
    UPROPERTY(EditAnywhere   , Category = "Bodies | Sail"                 , meta = (DisplayName = "Massa della Vela (Kg)"                                                                              , ClampMin = "90"   , ClampMax = "95"      )) double                           SAIL_MASS                  = 90.0;
    UPROPERTY(EditAnywhere   , Category = "Bodies | Sail"                 , meta = (DisplayName = "Proporzione della Vela"                                                                             , ClampMin = "1.0"  , ClampMax = "50.0"    )) double                           SAIL_SCALE                 = 12.0;

    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Posizione della Vela"                                                                                                                          )) FVector3d                        SailPosition               = FVector3d::Zero();
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Velocità della Vela (Km/s)"                                                                                                                    )) FVector3d                        SailVelocity               = FVector3d::Zero();
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Accelerazione della Vela (Km/s²)"                                                                                                              )) FVector3d                        SailAcceleration           = FVector3d::Zero(); 
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Forza Totale (N)"                                                                                                                              )) FVector3d                        TotalForce                 = FVector3d::Zero(); 
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Distanza della Vela dalla Terra (Km)"                                                                                                          )) double                           SailDistanceFromEarth      = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Distanza della Vela dal Sole (Km)"                                                                                                             )) double                           SailDistanceFromSun        = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Raggio dell'Orbita (Km)"                                                                                                                       )) double                           OrbitRadius                = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Angolo di Incidenza (deg)"                                                                                                                     )) double                           IncidenceAngle             = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Coseno dell'Angolo di Incidenza"                                                                                                               )) double                           CosTheta                   = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Normale della Vela"                                                                                                                            )) FVector3d                        SailNormal                 = FVector3d::Zero();
    UPROPERTY(VisibleAnywhere, Category = "Bodies | Sail | Live Telemetry", meta = (DisplayName = "Direzione del Sole"                                                                                                                            )) FVector3d                        SunDirection               = FVector3d::Zero();

    UPROPERTY(VisibleAnywhere, Category = "Physic | Solar Pressure"       , meta = (DisplayName = "Forza Solare (N)"                                                                                                                              )) FVector3d                        SolarForce                 = FVector3d::Zero();
    UPROPERTY(VisibleAnywhere, Category = "Physic | Solar Pressure"       , meta = (DisplayName = "Modulo della Forza Solare (N)"                                                                                                                 )) double                           SolarForceModule           = 0.0;
    UPROPERTY(EditAnywhere   , Category = "Physic | Solar Pressure"       , meta = (DisplayName = "Coefficiente di Riflettività"                                                                                                                  )) double                           REFLECTIVITY_COEFFICIENT   = 2.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Solar Pressure"       , meta = (DisplayName = "Pressione Solare (Pa)"                                                                                                                         )) double                           SolarPressure              = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Solar Pressure"       , meta = (DisplayName = "Versore della Forza Solare"                                                                                                                    )) FVector3d                        SolarForceVersor           = FVector3d::Zero();

    UPROPERTY(EditAnywhere   , Category = "Physic | Orbit Settings"       , meta = (DisplayName = "Tipo di Orbita Iniziale"                                                                                                                       )) EOrbitStartType                  OrbitType                  = EOrbitStartType::Geostationary;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Settings"       , meta = (DisplayName = "Quota LEO"      , EditCondition = "OrbitType == EOrbitStartType::LEO_ISS"       , EditConditionHides                                           )) double                           ORBIT_LEO                  = 400.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Settings"       , meta = (DisplayName = "Quota GPS"      , EditCondition = "OrbitType == EOrbitStartType::MEO_GPS"       , EditConditionHides                                           )) double                           ORBIT_GPS                  = 20200.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Settings"       , meta = (DisplayName = "Quota GEO"      , EditCondition = "OrbitType == EOrbitStartType::Geostationary" , EditConditionHides                                           )) double                           ORBIT_GEO                  = 35786.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Settings"       , meta = (DisplayName = "Distanza Lunare", EditCondition = "OrbitType == EOrbitStartType::LunarDistance" , EditConditionHides                                           )) double                           ORBIT_MOON                 = 384400.0;
    UPROPERTY(EditAnywhere   , Category = "Physic | Orbit Settings"       , meta = (DisplayName = "Quota Custom"   , EditCondition = "OrbitType == EOrbitStartType::CustomAltitude", EditConditionHides, ClampMin = "200.0", ClampMax = "500000.0")) double                           ORBIT_CUSTOM               = 400.0;

    UPROPERTY(VisibleAnywhere, Category = "Physic | Gravity Values"       , meta = (DisplayName = "Forza di Gravità (N)"                                                                                                                          )) FVector3d                        GravityForce               = FVector3d::Zero();
    UPROPERTY(VisibleAnywhere, Category = "Physic | Gravity Values"       , meta = (DisplayName = "Modulo della Forza di Gravità (N)"                                                                                                             )) double                           GravityForceModule         = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Gravity Values"       , meta = (DisplayName = "Versore della Forza di Gravità"                                                                                                                )) FVector3d                        GravityForceVersor         = FVector3d::Zero();

    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Velocity"       , meta = (DisplayName = "Velocità Orbitale Iniziale (Km/s)"                                                                                                             )) FVector3d                        InitialOrbitVelocity       = FVector3d::Zero();
    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Velocity"       , meta = (DisplayName = "Modulo della Velocità Orbitale Iniziale (Km/s)"                                                                                                )) double                           InitialOrbitVelocityModule = 0.0;
    UPROPERTY(VisibleAnywhere, Category = "Physic | Orbit Velocity"       , meta = (DisplayName = "Versore della Velocità Orbitale Iniziale"                                                                                                      )) FVector3d                        InitialOrbitVelocityVersor = FVector3d::Zero();

    UPROPERTY(VisibleAnywhere, Category = "RayCasting"                    , meta = (DisplayName = "Fotoni Attivi"                                                                                                                                 )) int32                            ActivePhotons              = 0;
    UPROPERTY(VisibleAnywhere, Category = "RayCasting"                    , meta = (DisplayName = "Fotoni Totali"                                                                                                                                 )) int32                            TotalPhotons               = 0;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    ASimulationManager* Manager;
    
    FString SailName;
    FString CsvFilePath;
    bool bCsvHeaderWritten = false;
    float RaycastTimer = 0.0f;
    float RaycastInterval = 0.2f;
    FString CsvBuffer;
    float CsvWriteTimer = 0.0f;

    bool InitializeManager();
    void InitializePhysicsProperties();
    void SetInitialPositions();
    void SetInitialRotations();
    void InitializeCSVReporting();
    void UpdateGravityForce();
    void UpdateSailRotation(float DeltaTime);
    //void AlignSailNormal();
    void UpdateSolarForce(float DeltaTime);
    void AppendDataToCSV(float DeltaTime);
    void DebugVisuals();
};
