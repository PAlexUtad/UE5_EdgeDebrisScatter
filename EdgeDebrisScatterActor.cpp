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
	SpreadDistance = 350.0f;;
	DebrisReach = 650.0f;
}

void AEdgeDebrisScatterActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

UInstancedStaticMeshComponent* AEdgeDebrisScatterActor::GetOrCreateISMC(UStaticMesh* Mesh)
{
	if (ISMCMap.Contains(Mesh))
		return ISMCMap[Mesh];

	UInstancedStaticMeshComponent* ISMC = NewObject<UInstancedStaticMeshComponent>(this);
	ISMC->SetStaticMesh(Mesh);
	ISMC->SetupAttachment(GetRootComponent());
	ISMC->RegisterComponent();
	ISMCMap.Add(Mesh, ISMC);
	return ISMC;
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
	FlushPersistentDebugLines(GetWorld());
}
void AEdgeDebrisScatterActor::Generate()
{
	// create tarray for hit results
	TArray<FHitResult> OutHits;
	
	// start and end locations
	FVector SweepStart = GetActorLocation();
	FVector SweepEnd = GetActorLocation();

	// create a collision sphere
	FCollisionShape MyColSphere = FCollisionShape::MakeSphere(Radius);

	// draw collision sphere
	DrawDebugSphere(GetWorld(), GetActorLocation(), MyColSphere.GetSphereRadius(), 50, FColor::Purple, false, 5.0f);
	
	// check if something got hit in the sweep
	bool isHit = GetWorld()->SweepMultiByChannel(OutHits, SweepStart, SweepEnd, FQuat::Identity, ECC_WorldStatic, MyColSphere);

	if (isHit)
	{
		// loop through TArray
		for (auto& Hit : OutHits)
		{
			float UpDot = FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector);
			if (FMath::Abs(UpDot) < 0.3f) // vertical face = edge
			{
				// Scatter N candidate points along this edge hit
				int32 NumSamples = FMath::RandRange(MinSamples, MaxSamples);
				for (int32 i = 0; i < NumSamples; i++)
				{
					// Random offset along the wall face and slightly away from it
					FVector TangentOffset = FVector::CrossProduct(Hit.ImpactNormal, FVector::UpVector);
					TangentOffset.Normalize();
            
					float AlongWall = FMath::FRandRange(-1.f, 1.f);
					AlongWall = FMath::Sign(AlongWall) * FMath::Pow(FMath::Abs(AlongWall), 0.5f) * SpreadDistance;
					float RawRand = FMath::FRandRange(0.f, 1.f);
					float BiasedRand = FMath::Pow(RawRand, 3.0f); // square it to bias toward 0
					float AwayFromWall = BiasedRand * DebrisReach;

					FVector SampleOrigin = Hit.ImpactPoint
						+ TangentOffset * AlongWall
						+ Hit.ImpactNormal * AwayFromWall  // pushes away from building
						+ FVector(0, 0, 200.f);  // lift up before tracing down

					FHitResult GroundHit;
					if (GetWorld()->LineTraceSingleByChannel(GroundHit, SampleOrigin, SampleOrigin - FVector(0,0,500), ECC_WorldStatic))
					{
						float Distance = FVector::Dist(GetActorLocation(), GroundHit.ImpactPoint);
						float Falloff = 1.0f - FMath::Clamp(Distance / Radius, 0.f, 1.f);

						// Randomly skip this point based on falloff
						// far points get culled more often
						if (FMath::FRandRange(0.f, 1.f) > Falloff)
							continue;
						
						if (DebrisMeshes.Num() > 0)
						{
							UStaticMesh* ChosenMesh = DebrisMeshes[FMath::RandRange(0, DebrisMeshes.Num() - 1)];
							UInstancedStaticMeshComponent* ISMC = GetOrCreateISMC(ChosenMesh);

							// Align to ground normal, random yaw
							FVector Up = GroundHit.ImpactNormal;
							FRotator Rotation = Up.Rotation();
							Rotation.Yaw += FMath::FRandRange(0.f, 360.f);

							// Scale slightly biased by falloff so closer pieces are a bit bigger
							float Scale = FMath::Lerp(MinScale, MaxScale, Falloff * FMath::FRandRange(0.5f, 1.f));

							FTransform InstanceTransform(Rotation, GroundHit.ImpactPoint, FVector(Scale));
							ISMC->AddInstance(InstanceTransform, true);
						}
						else
						{
							// if no mesh, draw a point
							DrawDebugPoint(GetWorld(), GroundHit.ImpactPoint, 10.f,  FColor::Red, false, 5.0f);
						}
					}
				}
			}
		}
	}
}