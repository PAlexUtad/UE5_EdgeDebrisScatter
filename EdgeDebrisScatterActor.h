#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EdgeDebrisScatterActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USphereComponent;

UCLASS()
class TOOLS_API AEdgeDebrisScatterActor : public AActor
{
	GENERATED_BODY()

public:
	AEdgeDebrisScatterActor();
	void OnConstruction(const FTransform& Transform);

	UFUNCTION(CallInEditor, Category="Debris Tool")
	void Generate();

	UFUNCTION(CallInEditor, Category="Debris Tool")
	void Clear();


protected:
	
	UPROPERTY(EditAnywhere, Category = "Debris Tool")
	float Radius;
	UPROPERTY(EditAnywhere, Category = "Debris Tool")
	int MinSamples;
	UPROPERTY(EditAnywhere, Category = "Debris Tool")
	int MaxSamples;
	UPROPERTY(EditAnywhere, Category = "Debris Tool")
	float SpreadDistance;
	UPROPERTY(EditAnywhere, Category = "Debris Tool")
	float DebrisReach;
	
	
	UPROPERTY(EditAnywhere, Category = "Debris")
	TArray<UStaticMesh*> DebrisMeshes;

	UPROPERTY(EditAnywhere, Category = "Debris")
	float MinScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Debris")
	float MaxScale = 1.5f;
	
private: 
	TMap<UStaticMesh*, UInstancedStaticMeshComponent*> ISMCMap;

	UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh);
};