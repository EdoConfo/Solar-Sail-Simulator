// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "SimulationManager.generated.h"

UCLASS()
class SIMULATION_API ASimulationManager : public AActor {
    GENERATED_BODY()

public:
    ASimulationManager();
    virtual void Tick(float DeltaTime) override;
    FVector3d GetEarthGravityAccelerationAt(FVector3d SailLocationUU, double SailDistanceFromEarthKm) const;
    double GetSolarPressureAt(FVector3d SailLocationUU, double SailDistanceFromSunKm) const;

    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Moltiplicatore del Tempo"         , ClampMin = "0.01", ClampMax = "10000" )) float                         TimeScale                  = 1.0f;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Moltiplicatore della Forza"       , ClampMin = "0.01", ClampMax = "10000" )) float                         ForceMultiplier            = 1.0f;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Risoluzione della Griglia"        , ClampMin = "1"   , ClampMax = "100"   )) int32                         GridResolution             = 10;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Abilita Gravità"                                                          )) bool                          EnableGravity              = true;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Abilita Pressione Solare"                                                 )) bool                          EnableSolarPressure        = true;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Vela Doppia Faccia"                                                       )) bool                          DoubleSidedSail            = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Abilita Log CSV"                                                          )) bool                          EnableCSVLogging           = true;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Telemetria Live"                                                   )) bool                          ShowDebugTelemetry         = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Fotoni"                                                            )) bool                          ShowPhotonDebug            = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Distanza Vela - Terra"                                             )) bool                          ShowSailEarthDistanceDebug = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Distanza Vela - Sole"                                              )) bool                          ShowSailSunDistanceDebug   = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Distanza Terra - Sole"                                             )) bool                          ShowEarthSunDistanceDebug  = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Normale della Vela"                                                )) bool                          ShowSailNormalDebug        = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Forza Solare"                                                      )) bool                          ShowSolarForceDebug        = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Forza di Gravità"                                                  )) bool                          ShowGravityForceDebug      = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Settings"        , meta = (DisplayName = "Mostra Forza Totale"                                                      )) bool                          ShowTotalForceDebug        = false;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Bodies | Earth"  , meta = (DisplayName = "Mesh della Terra"                                                         )) TObjectPtr<AActor>            EARTH_MESH_ACTOR           = nullptr;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Earth"  , meta = (DisplayName = "Posizione della Terra"                                                    )) FVector3d                     EARTH_POSITION             = FVector3d(0.0, 0.0, 0.0);
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Earth"  , meta = (DisplayName = "Raggio della Terra (Km)"                                                  )) double                        EARTH_RADIUS               = 6371.0;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Earth"  , meta = (DisplayName = "Massa della Terra (Kg)"                                                   )) double                        EARTH_MASS                 = 5.9722e24;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Earth"  , meta = (DisplayName = "Proporzione della Terra"                                                  )) double                        EARTH_SCALE                = 12756.274;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Bodies | Sun"    , meta = (DisplayName = "Mesh del Sole"                                                            )) TObjectPtr<AActor>            SUN_MESH_ACTOR             = nullptr;
    UPROPERTY(EditAnywhere   , Category = "Simulation Parameters | Bodies | Sun"    , meta = (DisplayName = "Luce del Sole"                                                            )) TObjectPtr<ADirectionalLight> SUN_LIGHT_ACTOR            = nullptr;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Sun"    , meta = (DisplayName = "Posizione del Sole"                                                       )) FVector3d                     SUN_POSITION               = FVector3d(149597870.7, 0.0, 0.0);
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Sun"    , meta = (DisplayName = "Raggio del Sole (Km)"                                                     )) double                        SUN_RADIUS                 = 696340.0;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Sun"    , meta = (DisplayName = "Massa del Sole (Kg)"                                                      )) double                        SUN_MASS                   = 1.9891e30;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Bodies | Sun"    , meta = (DisplayName = "Proporzione del Sole"                                                     )) double                        SUN_SCALE                  = 696340.0;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Physic Constants", meta = (DisplayName = "Costante Gravitazionale (N·m²/kg)"                                        )) double                        GRAVITATIONAL_CONSTANT     = 6.67430e-11;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Physic Constants", meta = (DisplayName = "Velocità della Luce (Km/s)"                                               )) double                        SPEED_OF_LIGHT             = 299792.458;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Physic Constants", meta = (DisplayName = "Unità Astronomica (Km)"                                                   )) double                        ASTRONOMICAL_UNIT          = 149597870.7;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Physic Constants", meta = (DisplayName = "Pi Greco"                                                                 )) double                        PI_GREEK                   = 3.14159265359;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Physic Constants", meta = (DisplayName = "Luminosità Solare (W)"                                                    )) double                        SOLAR_LUMINOSITY           = 3.828e26;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Physic Constants", meta = (DisplayName = "Orbita Geostazionaria (Km)"                                               )) double                        GEOSTATIONARY_ORBIT        = 35786.0;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Unit Conversions", meta = (DisplayName = "Unreal Units to Meters"                                                   )) float                         UU_TO_METERS               = 1.0f;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Unit Conversions", meta = (DisplayName = "Meters to Unreal Units"                                                   )) float                         METERS_TO_UU               = 1.0f;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Unit Conversions", meta = (DisplayName = "Kilometers to Unreal Units"                                               )) float                         KM_TO_UU                   = 1000.0f;
    UPROPERTY(VisibleAnywhere, Category = "Simulation Parameters | Unit Conversions", meta = (DisplayName = "Unreal Units to Kilometers"                                               )) float                         UU_TO_KM                   = 0.001f;

    static ASimulationManager* Instance;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
