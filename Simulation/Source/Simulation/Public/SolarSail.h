 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "SimulationManager.h"
#include "SolarSail.generated.h"

UCLASS()
class SIMULATION_API ASolarSail : public AActor {
	GENERATED_BODY()
	
public:
	ASolarSail();
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Solar Sail") UStaticMeshComponent* SailMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")  FVector CurrentSolarForce;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")  float CurrentIncidenceAngle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")  float CurrentDistanceAU;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System") ADirectionalLight* SunLightActor;

protected:
	virtual void BeginPlay() override;

private:
	float CachedSolarPressure;
	float CachedAuScale;
	FString CsvFilePath;
	
	bool bCsvHeaderWritten = false;
	void AppendDataToCSV(float DeltaTime);
    void FindSunInScene();
    void UpdateSolarForce(float DeltaTime);
	void UpdateGravityForce();
	void AlignSailNormal();
};