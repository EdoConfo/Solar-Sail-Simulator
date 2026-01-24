// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSail.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"

ASolarSail::ASolarSail() {
    PrimaryActorTick.bCanEverTick = true;
    SailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SailMesh"));
    RootComponent = SailMesh;
    SailMesh->SetSimulatePhysics(true);
    SailMesh->SetEnableGravity(false);
    SailMesh->SetLinearDamping(0.0f);
    SailMesh->SetAngularDamping(0.0f);
    SailArea = 1.0f; 
    TotalMass = 1.0f; 
    SailMesh->SetMassOverrideInKg(NAME_None, TotalMass, true);
    GridResolution = 5; 
    bShowPhotonDebug = false; 
    bShowSunDistanceDebug = true;
    bIsDoubleSided = false;
    CachedSolarPressure = 0.0f;
    CachedAuScale = 1.0f;
    ForceMultiplier = 1.0f;
}

void ASolarSail::BeginPlay() {
    Super::BeginPlay();
    
    if (ASimulationManager::Instance) {
        TotalMass = ASimulationManager::Instance->SailMass;
        SailArea = ASimulationManager::Instance->SailArea;
        GridResolution = ASimulationManager::Instance->GridResolution;
        bShowPhotonDebug = ASimulationManager::Instance->bShowPhotonDebug;
        bShowSunDistanceDebug = ASimulationManager::Instance->bShowSunDistanceDebug;
        bIsDoubleSided = ASimulationManager::Instance->bIsDoubleSided;
        ForceMultiplier = ASimulationManager::Instance->ForceMultiplier;
        CachedSolarPressure = ASimulationManager::Instance->SolarPressureAt1AU;
        CachedAuScale = ASimulationManager::Instance->AuToUnrealScale;
    } else {
        CachedSolarPressure = 0.00000456f;
        CachedAuScale = 10000.0f;
        SailArea = 32.0f;
    }

    if (SailMesh) {
        SailMesh->SetMassOverrideInKg(NAME_None, TotalMass, true);
    }

    FindSunInScene();

    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Inizializzazione Vela Solare: Completata!"));
    }
}

void ASolarSail::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);    

    if (ASimulationManager::Instance) {
        GridResolution = ASimulationManager::Instance->GridResolution;
        bShowPhotonDebug = ASimulationManager::Instance->bShowPhotonDebug;
        bShowSunDistanceDebug = ASimulationManager::Instance->bShowSunDistanceDebug;
        bIsDoubleSided = ASimulationManager::Instance->bIsDoubleSided;
        ForceMultiplier = ASimulationManager::Instance->ForceMultiplier;
        SailArea = ASimulationManager::Instance->SailArea;
        CachedSolarPressure = ASimulationManager::Instance->SolarPressureAt1AU;
        CachedAuScale = ASimulationManager::Instance->AuToUnrealScale;
    }

    UpdateSolarForce(DeltaTime);
}

void ASolarSail::FindSunInScene() {
    if (SunLightActor) {
        return;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0) {
        SunLightActor = Cast<ADirectionalLight>(FoundActors[0]);
    } else {
        UE_LOG(LogTemp, Error, TEXT("ERRORE: Manca il Sole (DirectionalLight)!"));
    }
}

void ASolarSail::UpdateSolarForce(float DeltaTime)
{
    if (!SunLightActor || !SailMesh) {
        return;
    }

    FVector SailOrigin = GetActorLocation(); 
    FVector SunPosition = SunLightActor->GetActorLocation(); 
    FVector SailNormal = GetActorUpVector();
    FVector SailRight = GetActorRightVector();
    FVector SailForward = GetActorForwardVector();
    FVector MinBounds, MaxBounds;
    SailMesh->GetLocalBounds(MinBounds, MaxBounds); 
    FVector BoxExtent = (MaxBounds - MinBounds) * 0.5f;
    FVector ActorScale = GetActorScale3D();
    float LengthCm = BoxExtent.X * 2.0f * ActorScale.X; 
    float WidthCm = BoxExtent.Y * 2.0f * ActorScale.Y;
    float StepX = LengthCm / (float)GridResolution;
    float StepY = WidthCm / (float)GridResolution;
    float StartOffsetX = -(LengthCm / 2.0f) + (StepX / 2.0f);
    float StartOffsetY = -(WidthCm / 2.0f) + (StepY / 2.0f);
    float SubArea = SailArea / (float)(GridResolution * GridResolution);
    FVector TotalForce = FVector::ZeroVector;
    int32 ActivePhotons = 0; 
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); 

    for (int32 x = 0; x < GridResolution; x++) {
        for (int32 y = 0; y < GridResolution; y++) {
            float CurrentLocX = StartOffsetX + (x * StepX);
            float CurrentLocY = StartOffsetY + (y * StepY);
            FVector SampleLocation = SailOrigin + (SailForward * CurrentLocX) + (SailRight * CurrentLocY);
            FVector LightDirection = (SampleLocation - SunPosition).GetSafeNormal();
            float CosTheta = FVector::DotProduct(-LightDirection, SailNormal);
            FVector EffectiveNormal = SailNormal;

            if (bIsDoubleSided) {
                if (CosTheta < 0) {
                    EffectiveNormal = -SailNormal;
                    CosTheta = -CosTheta;
                }
            } else {
                if (CosTheta <= 0) {
                    continue;
                }
            }

            FHitResult HitResult;
            bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, SunPosition, SampleLocation, ECC_Visibility, QueryParams);

            if (!bHit) {
                float Distance = FVector::Dist(SunPosition, SampleLocation);
                float DistanceInAU = Distance / CachedAuScale;
                float SafeDistance = FMath::Max(DistanceInAU, 0.001f);
                float LocalPressure = CachedSolarPressure / (SafeDistance * SafeDistance);
                float ForceMagnitude = 2.0f * LocalPressure * SubArea * (CosTheta * CosTheta);
                ForceMagnitude *= ForceMultiplier; 
                FVector SubForce = -EffectiveNormal * ForceMagnitude; 
                FVector UnrealSubForce = SubForce * 100.0f;

                if (SailMesh->IsSimulatingPhysics()) {
                    SailMesh->AddForceAtLocation(UnrealSubForce, SampleLocation);
                }

                TotalForce += SubForce;
                ActivePhotons++;

                if (bShowPhotonDebug) {
                    DrawDebugLine(GetWorld(), SampleLocation - (LightDirection * 500.0f), SampleLocation, FColor::Green, false, -1.0f, 0, 0.5f);
                }
            } else if (bShowPhotonDebug) {
                 DrawDebugLine(GetWorld(), HitResult.Location, SampleLocation, FColor::Red, false, -1.0f, 0, 0.5f);
            }
        }
    }

    CurrentSolarForce = TotalForce;
    CurrentDistanceAU = FVector::Dist(SunPosition, SailOrigin) / CachedAuScale;
    FVector CenterLightDir = (SailOrigin - SunPosition).GetSafeNormal();
    CurrentIncidenceAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Max(0.0f, FVector::DotProduct(-CenterLightDir, SailNormal))));

    if (bShowSunDistanceDebug) {
        DrawDebugLine(GetWorld(), SunPosition, SailOrigin, FColor::Yellow, false, -1.0f, 0, 1.5f);
    }

    if (GEngine) {
        float AvgForcePerRay = 0.0f;

        if (ActivePhotons > 0.0f) {
            AvgForcePerRay = CurrentSolarForce.Size() / (float)ActivePhotons;
        }

        int32 TotalRays = GridResolution * GridResolution;
        int32 InactiveRays = TotalRays - ActivePhotons;
        FString Msg1 = FString::Printf(TEXT("------ Solar Sail Data ------"));
        FString Msg2 = FString::Printf(TEXT("Grid Resolution:  %dx%d (%d Total)"), GridResolution, GridResolution, TotalRays);
        FString Msg3 = FString::Printf(TEXT("Active Rays:      %d (%.1f%%)"), ActivePhotons, ((float)ActivePhotons/(float)TotalRays)*100.0f);
        FString Msg4 = FString::Printf(TEXT("Blocked Rays:     %d (%.1f%%)"), InactiveRays, ((float)InactiveRays/(float)TotalRays)*100.0f);
        FString Msg5 = FString::Printf(TEXT("Total Force:      %.2e N"), CurrentSolarForce.Size());
        FString Msg6 = FString::Printf(TEXT("Force per Ray:    %.2e N"), AvgForcePerRay);
        FString Msg7 = FString::Printf(TEXT("Incidence Angle:  %.1f deg"), CurrentIncidenceAngle);
        FString Msg8 = FString::Printf(TEXT("Sail Weight:      %.2f kg"), TotalMass);
        FString Msg9 = FString::Printf(TEXT("Distance Sun:     %.4f AU"), CurrentDistanceAU);
        
        GEngine->AddOnScreenDebugMessage(17, 0.0f, FColor::Yellow, Msg9);
        GEngine->AddOnScreenDebugMessage(16, 0.0f, FColor::White, Msg8);
        GEngine->AddOnScreenDebugMessage(15, 0.0f, FColor::White, Msg7);
        GEngine->AddOnScreenDebugMessage(14, 0.0f, FColor::White, Msg6);
        GEngine->AddOnScreenDebugMessage(13, 0.0f, FColor::White, Msg5);
        GEngine->AddOnScreenDebugMessage(18, 0.0f, FColor::Red, Msg4);
        GEngine->AddOnScreenDebugMessage(12, 0.0f, ActivePhotons > 0 ? FColor::Green : FColor::Red, Msg3);
        GEngine->AddOnScreenDebugMessage(11, 0.0f, FColor::White, Msg2);
        GEngine->AddOnScreenDebugMessage(10, 0.0f, FColor::Cyan, Msg1);
    }
}