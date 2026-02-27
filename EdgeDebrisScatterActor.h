#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EdgeDebrisScatterActor.generated.h"

class UInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct FDebrisMeshEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1", ToolTip = "Relative spawn frequency. Higher = appears more often."))
    float Weight = 1.0f;
};

UCLASS()
class TOOLS_API AEdgeDebrisScatterActor : public AActor
{
    GENERATED_BODY()

public:
    AEdgeDebrisScatterActor();
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(CallInEditor, Category = "Debris Tool")
    void Generate();

    UFUNCTION(CallInEditor, Category = "Debris Tool")
    void Clear();

    UFUNCTION(CallInEditor, Category = "Debris Tool")
    void PrintInstanceCount();

protected:

    // Detection

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Detection", meta = (ClampMin = "0", ToolTip = "Radius of the sphere used to detect nearby geometry."))
    float Radius = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Dot product threshold for what counts as a vertical edge face. Lower = stricter."))
    float EdgeNormalThreshold = 0.3f;

    // Distribution

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "0", ToolTip = "Minimum scatter samples per edge hit."))
    int32 MinSamples = 300;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "0", ToolTip = "Maximum scatter samples per edge hit."))
    int32 MaxSamples = 600;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "0", ToolTip = "Hard cap on total instances to prevent freezing on large scenes."))
    int32 MaxTotalInstances = 2000;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "0", ToolTip = "How far debris spreads along the wall face."))
    float SpreadDistance = 350.0f;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "0", ToolTip = "How far debris spills outward away from the building."))
    float DebrisReach = 650.0f;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "0", ToolTip = "Minimum world distance between any two debris pieces."))
    float MinSpacing = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Distribution", meta = (ClampMin = "1.0", ToolTip = "Higher values cluster debris tighter to the edge. 1 = linear, 3 = very tight."))
    float FalloffExponent = 3.0f;

    // Randomness

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Randomness", meta = (ToolTip = "Fix this seed to get reproducible results. Change it to get a different scatter pattern."))
    int32 RandomSeed = 42;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Randomness", meta = (ToolTip = "If true, OnConstruction will re-generate automatically when you move or modify the actor."))
    bool bAutoRefresh = false;

    // Meshes

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Meshes", meta = (ToolTip = "Set of meshes to scatter. Each entry has a weight controlling relative frequency."))
    TArray<FDebrisMeshEntry> DebrisMeshes;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Meshes", meta = (ClampMin = "0"))
    float MinScale = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Debris Tool|Meshes", meta = (ClampMin = "0"))
    float MaxScale = 1.5f;

private:
    bool bIsGenerating = false;
    TMap<UStaticMesh*, UInstancedStaticMeshComponent*> ISMCMap;
    TArray<FVector> PlacedPoints;
    int32 LastInstanceCount = 0;
    void TrySpawnAtGroundHit(FVector SampleOrigin, FVector ActorLocation, FVector WallPoint);
    UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh);
    UStaticMesh* PickWeightedMesh();
};
