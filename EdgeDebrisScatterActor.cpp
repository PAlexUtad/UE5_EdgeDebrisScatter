#include "EdgeDebrisScatterActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"

AEdgeDebrisScatterActor::AEdgeDebrisScatterActor()
{
    Radius = 500.0f;
    MinSamples = 300;
    MaxSamples = 600;
    SpreadDistance = 350.0f;
    DebrisReach = 650.0f;
    EdgeNormalThreshold = 0.3f;
    MinSpacing = 15.0f;
    FalloffExponent = 3.0f;
    MaxTotalInstances = 2000;
    RandomSeed = 42;
    bAutoRefresh = false;
    MinScale = 0.5f;
    MaxScale = 1.5f;
    LastInstanceCount = 0;
}

void AEdgeDebrisScatterActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (bAutoRefresh && !bIsGenerating)
    {
        bIsGenerating = true;
        Clear();
        for (const FDebrisMeshEntry& Entry : DebrisMeshes)
        {
            if (Entry.Mesh)
                GetOrCreateISMC(Entry.Mesh);
        }
        Generate();
        bIsGenerating = false;
    }
}

UInstancedStaticMeshComponent* AEdgeDebrisScatterActor::GetOrCreateISMC(UStaticMesh* Mesh)
{
    if (ISMCMap.Contains(Mesh))
        return ISMCMap[Mesh];

    UInstancedStaticMeshComponent* ISMC = NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());
    ISMC->SetStaticMesh(Mesh);
    ISMC->SetupAttachment(GetRootComponent());
    ISMC->SetFlags(RF_Transactional);
    ISMC->RegisterComponent();
    AddInstanceComponent(ISMC);
    ISMCMap.Add(Mesh, ISMC);
    return ISMC;
}

UStaticMesh* AEdgeDebrisScatterActor::PickWeightedMesh()
{
    if (DebrisMeshes.Num() == 0) return nullptr;

    float TotalWeight = 0.f;
    for (const FDebrisMeshEntry& Entry : DebrisMeshes)
        TotalWeight += Entry.Weight;

    float Roll = FMath::FRandRange(0.f, TotalWeight);
    float Accumulated = 0.f;
    for (const FDebrisMeshEntry& Entry : DebrisMeshes)
    {
        Accumulated += Entry.Weight;
        if (Roll <= Accumulated)
            return Entry.Mesh;
    }
    return DebrisMeshes.Last().Mesh;
}

void AEdgeDebrisScatterActor::Clear()
{
    TArray<UInstancedStaticMeshComponent*> ISMCs;
    GetComponents<UInstancedStaticMeshComponent>(ISMCs);
    for (UInstancedStaticMeshComponent* ISMC : ISMCs)
    {
        ISMC->ClearInstances();
        ISMC->DestroyComponent();
    }
    ISMCMap.Empty();
    PlacedPoints.Empty();
    LastInstanceCount = 0;
    FlushPersistentDebugLines(GetWorld());
}

void AEdgeDebrisScatterActor::TrySpawnAtGroundHit(FVector SampleOrigin, FVector ActorLocation, FVector WallPoint)
{
    FHitResult GroundHit;
    if (!GetWorld()->LineTraceSingleByChannel(GroundHit, SampleOrigin, SampleOrigin - FVector(0, 0, 500), ECC_WorldStatic))
        return;

    // Falloff from the wall edge, not the actor center
    float DistFromWall = FVector::Dist2D(GroundHit.ImpactPoint, WallPoint);
    float Falloff = 1.0f - FMath::Clamp(DistFromWall / DebrisReach, 0.f, 1.f);

  
    float CullChance = FMath::Pow(Falloff, 0.5f);
    if (FMath::FRandRange(0.f, 1.f) > CullChance) return;

    for (const FVector& Existing : PlacedPoints)
    {
        if (FVector::DistSquared(Existing, GroundHit.ImpactPoint) < MinSpacing * MinSpacing)
            return;
    }

    PlacedPoints.Add(GroundHit.ImpactPoint);

    if (DebrisMeshes.Num() > 0)
    {
        UStaticMesh* ChosenMesh = PickWeightedMesh();
        if (!ChosenMesh) return;

        UInstancedStaticMeshComponent* ISMC = GetOrCreateISMC(ChosenMesh);
        FRotator Rotation = GroundHit.ImpactNormal.Rotation();
        Rotation.Yaw += FMath::FRandRange(0.f, 360.f);

        FVector ScaleVec(
            FMath::FRandRange(MinScale, MaxScale),
            FMath::FRandRange(MinScale, MaxScale),
            FMath::FRandRange(MinScale * 0.5f, MaxScale * 0.5f)
        );

        ISMC->AddInstance(FTransform(Rotation, GroundHit.ImpactPoint, ScaleVec), true);
        LastInstanceCount++;
    }
    else
    {
        DrawDebugPoint(GetWorld(), GroundHit.ImpactPoint, 10.f,
            FColor::MakeRedToGreenColorFromScalar(Falloff), false, 5.0f);
    }
}


void AEdgeDebrisScatterActor::PrintInstanceCount()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("EdgeDebrisScatter: %d instances placed"), LastInstanceCount));
    }
    UE_LOG(LogTemp, Log, TEXT("EdgeDebrisScatter: %d instances placed"), LastInstanceCount);
}

void AEdgeDebrisScatterActor::Generate()
{
    if (DebrisMeshes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("EdgeDebrisScatterActor: No debris meshes assigned!"));
    }

    for (const FDebrisMeshEntry& Entry : DebrisMeshes)
    {
        if (Entry.Mesh)
            GetOrCreateISMC(Entry.Mesh);
    }

    FMath::RandInit(RandomSeed);

    TArray<FHitResult> OutHits;
    FVector ActorLocation = GetActorLocation();
    FCollisionShape MyColSphere = FCollisionShape::MakeSphere(Radius);

    DrawDebugSphere(GetWorld(), ActorLocation, MyColSphere.GetSphereRadius(), 50, FColor::Purple, false, 5.0f);

    bool bIsHit = GetWorld()->SweepMultiByChannel(OutHits, ActorLocation, ActorLocation, FQuat::Identity, ECC_WorldStatic, MyColSphere);

    if (!bIsHit) return;

    for (int32 HitIdx = 0; HitIdx < OutHits.Num(); HitIdx++)
{
    if (LastInstanceCount >= MaxTotalInstances) break;

    FHitResult& Hit = OutHits[HitIdx];
    float UpDot = FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector);
    if (FMath::Abs(UpDot) >= EdgeNormalThreshold) continue;

    bool bFoundNeighbor = false;

    for (int32 NeighborIdx = HitIdx + 1; NeighborIdx < OutHits.Num(); NeighborIdx++)
    {
        if (LastInstanceCount >= MaxTotalInstances) break;

        FHitResult& OtherHit = OutHits[NeighborIdx];
        float OtherUpDot = FVector::DotProduct(OtherHit.ImpactNormal, FVector::UpVector);
        if (FMath::Abs(OtherUpDot) >= EdgeNormalThreshold) continue;

        float NeighborDist = FVector::Dist(Hit.ImpactPoint, OtherHit.ImpactPoint);
        if (NeighborDist > SpreadDistance) continue;

        bFoundNeighbor = true;

        // neighbor interpolation path
        int32 NumSamples = FMath::RandRange(MinSamples, MaxSamples);
        for (int32 i = 0; i < NumSamples; i++)
        {
            if (LastInstanceCount >= MaxTotalInstances) break;

            float T = FMath::FRandRange(0.f, 1.f);
            FVector InterpolatedPoint = FMath::Lerp(Hit.ImpactPoint, OtherHit.ImpactPoint, T);
            FVector BlendedNormal = FMath::Lerp(Hit.ImpactNormal, OtherHit.ImpactNormal, T).GetSafeNormal();

            float AwayFromWall = FMath::Pow(FMath::FRandRange(0.f, 1.f), FalloffExponent) * DebrisReach;

            FVector SampleOrigin = InterpolatedPoint
                + BlendedNormal * AwayFromWall
                + FVector(0, 0, 200.f);

            TrySpawnAtGroundHit(SampleOrigin, ActorLocation, InterpolatedPoint);
        }
    }

    if (!bFoundNeighbor)
    {
        int32 NumSamples = FMath::RandRange(MinSamples, MaxSamples);
        for (int32 i = 0; i < NumSamples; i++)
        {
            if (LastInstanceCount >= MaxTotalInstances) break;

            // Random angle in a circle instead of straight tangent
            float Angle = FMath::FRandRange(0.f, 2.f * PI);
            FVector RandomDir = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
    
            // Blend between random circle and wall normal so it still biases away from surface
            FVector BiasedDir = FMath::Lerp(RandomDir, FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.f), 0.5f).GetSafeNormal();

            float AwayFromWall = FMath::Pow(FMath::FRandRange(0.f, 1.f), FalloffExponent) * DebrisReach;

            FVector SampleOrigin = Hit.ImpactPoint
                + BiasedDir * AwayFromWall
                + FVector(0, 0, 200.f);

            TrySpawnAtGroundHit(SampleOrigin, ActorLocation, Hit.ImpactPoint);
        }
    }
}

    UE_LOG(LogTemp, Log, TEXT("EdgeDebrisScatter: %d instances placed."), LastInstanceCount);
}
