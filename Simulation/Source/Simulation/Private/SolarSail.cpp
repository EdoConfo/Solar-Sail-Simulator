// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSail.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"

#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

ASolarSail::ASolarSail() {
    PrimaryActorTick.bCanEverTick = true;
    SailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SailMesh"));
    RootComponent = SailMesh;

    SailMesh->SetSimulatePhysics(true);
    SailMesh->SetEnableGravity(false);
    SailMesh->SetLinearDamping(0.0f);
    SailMesh->SetAngularDamping(0.0f);

    CachedSolarPressure = 0.0f;
    CachedAuScale = 1.0f;
}

void ASolarSail::BeginPlay() {
    Super::BeginPlay();

    SetInitialPositions();

    if (ASimulationManager::Instance) {
        CachedSolarPressure = ASimulationManager::Instance->SolarPressureAt1AU;
        CachedAuScale = ASimulationManager::Instance->AuToUnrealScale;
        if (SailMesh) {
            SailMesh->SetMassOverrideInKg(NAME_None, ASimulationManager::Instance->SailMass, true);
        }
    } else {
        CachedSolarPressure = 0.00000456f;
        CachedAuScale = 10000.0f;
        if (SailMesh) {
            SailMesh->SetMassOverrideInKg(NAME_None, 1.0f, true);
        }
    }

    FindSunInScene();

    FString ProjectDir = FPaths::ProjectDir();
    FString AnalysisDir = ProjectDir + TEXT("../Analysis/");
    IFileManager::Get().MakeDirectory(*AnalysisDir, true);
    CsvFilePath = AnalysisDir + TEXT("SolarSailData.csv");
    bCsvHeaderWritten = false;
    AppendDataToCSV(0.0f);

    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Inizializzazione Vela Solare: Completata!"));
    }
}

void ASolarSail::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);    

    if (ASimulationManager::Instance) {
        CachedSolarPressure = ASimulationManager::Instance->SolarPressureAt1AU;
        CachedAuScale = ASimulationManager::Instance->AuToUnrealScale;
    }

    UpdateGravityForce();

    AlignSailNormal();

    UpdateSolarForce(DeltaTime);
    AppendDataToCSV(DeltaTime);
}

void ASolarSail::UpdateGravityForce() {
    if (!SailMesh) {
        return;
    }

    if (!ASimulationManager::Instance) {
        return;
    }

    double G = ASimulationManager::Instance->GravitationalConstant;
    double EarthMass = ASimulationManager::Instance->EarthMass;
    double mVela = ASimulationManager::Instance->SailMass;

    FVector PosVela = GetActorLocation();
    FVector r_vec = PosVela - EarthPosition;
    double r = r_vec.Size();

    if (r < 1.0) {
        return;
    }

    double forza = G * EarthMass * mVela / (r * r);
    FVector forzaDir = -r_vec.GetSafeNormal();
    FVector forzaGrav = forzaDir * forza;

    FVector forzaGravUnreal = forzaGrav * 100.0;
    SailMesh->AddForce(forzaGravUnreal);
}

void ASolarSail::AlignSailNormal() {
    FVector PosVela = GetActorLocation();
    FVector TerraToVela = PosVela - EarthPosition;
    FVector Arbitrary = FVector::UpVector;
    if (FMath::Abs(FVector::DotProduct(TerraToVela.GetSafeNormal(), Arbitrary)) > 0.99f) {
        Arbitrary = FVector::RightVector;
    }
    FVector NormalVela = FVector::CrossProduct(TerraToVela, Arbitrary).GetSafeNormal();
    FRotationMatrix RotMat = FRotationMatrix::MakeFromXZ(NormalVela, TerraToVela.GetSafeNormal());
    FRotator NewRotation = RotMat.Rotator();
    SetActorRotation(NewRotation);
}

void ASolarSail::SetInitialPositions() {
    if (!ASimulationManager::Instance) return;

    EarthPosition = FVector(0.0f, 0.0f, 0.0f);
    FVector OrbitDirection = FVector::ForwardVector;

    float InitialDistanceKm = ASimulationManager::Instance->InitialSailDistanceKm;
    double G = ASimulationManager::Instance->GravitationalConstant;
    double EarthMass = ASimulationManager::Instance->EarthMass;
    double SunDistCm = ASimulationManager::Instance->SunDistanceCm;

    float InitialDistanceCm = InitialDistanceKm * 1e5f;
    FVector SailPos = EarthPosition + OrbitDirection * InitialDistanceCm;
    SetActorLocation(SailPos);

    double r_m = static_cast<double>(InitialDistanceKm) * 1000.0;
    double v_orb = sqrt(G * EarthMass / r_m);
    float v_orb_cm_s = static_cast<float>(v_orb * 100.0);

    FVector TangentDir = FVector::UpVector ^ OrbitDirection;
    TangentDir = TangentDir.GetSafeNormal();
    if (SailMesh) {
        SailMesh->SetPhysicsLinearVelocity(TangentDir * v_orb_cm_s);
    }

    FVector SunDirection = FVector::UpVector;
    SunPosition = EarthPosition + SunDirection * SunDistCm;

    if (SunLightActor)
    {
        SunLightActor->SetActorLocation(SunPosition);
    }
}

void ASolarSail::AppendDataToCSV(float DeltaTime) {
    if (!bCsvHeaderWritten) {
        FString Header = TEXT("Time,PosX,PosY,PosZ,VelX,VelY,VelZ,ForceX,ForceY,ForceZ,IncidenceAngle,DistanceAU\n");
        FFileHelper::SaveStringToFile(Header, *CsvFilePath);
        bCsvHeaderWritten = true;
    }

    float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    FVector Pos = GetActorLocation();
    FVector Vel = SailMesh && SailMesh->IsSimulatingPhysics() ? SailMesh->GetPhysicsLinearVelocity() : FVector::ZeroVector;
    FVector Force = CurrentSolarForce;
    float Angle = CurrentIncidenceAngle;
    float Dist = CurrentDistanceAU;

    FString Line = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.5f\n"), Time, Pos.X, Pos.Y, Pos.Z, Vel.X, Vel.Y, Vel.Z, Force.X, Force.Y, Force.Z, Angle, Dist);

    FFileHelper::SaveStringToFile(Line, *CsvFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
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

void ASolarSail::UpdateSolarForce(float DeltaTime) {
    if (!SunLightActor || !SailMesh) {
        return;
    }

    int32 GridResolution = 1;
    float SailArea = 1.0f;
    float ForceMultiplier = 1.0f;
    bool bIsDoubleSided = false;
    bool bShowPhotonDebug = false;
    bool bShowSunDistanceDebug = false;

    if (ASimulationManager::Instance) {
        GridResolution = ASimulationManager::Instance->GridResolution;
        SailArea = ASimulationManager::Instance->SailArea;
        ForceMultiplier = ASimulationManager::Instance->ForceMultiplier;
        bIsDoubleSided = ASimulationManager::Instance->bIsDoubleSided;
        bShowPhotonDebug = ASimulationManager::Instance->bShowPhotonDebug;
        bShowSunDistanceDebug = ASimulationManager::Instance->bShowSunDistanceDebug;
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