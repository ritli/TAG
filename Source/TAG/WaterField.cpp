// Fill out your copyright notice in the Description page of Project Settings.

#include "WaterField.h"
#include "CowCharacter.h"

// Sets default values
AWaterField::AWaterField()
{
	PrimaryActorTick.bCanEverTick = true;

	WaterFieldBox = CreateDefaultSubobject<UBoxComponent>("Water Bounding Box", false);
	WaterFieldBox->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	WaterFieldBox->OnComponentBeginOverlap.AddDynamic(this, &AWaterField::BeginOverlap);
}

void AWaterField::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWaterField::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	uint8 Len = FloatingActors.Num();
	for (uint8 i = 0; i < Len; i++)
	{
		if (FloatingActors[i] != NULL) {

			AActor* actor = FloatingActors[i];

			FVector location = actor->GetActorLocation();

			FVector multi = FloatSimulationMagnitude;
			FVector offset = FVector(WaveSimScalar * multi.X, -WaveSimScalar * multi.Y, WaveSimScalar * multi.Z);

			actor->SetActorLocation(location + CurrentVector * DeltaTime, true);
			actor->SetActorLocation(actor->GetActorLocation() + offset - FloatingLocations[i]);

			//UE_LOG(LogTemp, Warning, TEXT("Z offset = %f"), (offset).Z);

			FloatingLocations[i] = offset;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("WaveSim = %f"), WaveSimScalar);

	float toLerp = WaveMagnitude;

	if (!bPositiveWave) {
		toLerp = -toLerp;
	}

	WaveSimAlpha += DeltaTime * WaveSimMult;

	WaveSimScalar = FMath::InterpCircularInOut(WaveSimScalar, toLerp, WaveSimAlpha);

	if (WaveSimAlpha > 1) {
		WaveSimAlpha = 0;
		bPositiveWave = !bPositiveWave;
	}
}

void AWaterField::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	//On actor enter check for floating tag, add to array and disable physics
	if (OtherActor->ActorHasTag("Floating")) {

		UE_LOG(LogTemp, Warning, TEXT("Floating actor entered"));

		uint8 Len = FloatingActors.Num();
		for (uint8 i = 0; i < Len; i++) {
			if (FloatingActors[i]->GetName() == OtherActor->GetName()) {
				UE_LOG(LogTemp, Warning, TEXT("Actor already in water"));

				return;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Actor has been added to water"));

		FloatingActors.Add(OtherActor);
		FloatingLocations.Add(FVector(0, 0, 0));

		//Physics disabling and the like handled in blueprint
		//Event called here
		OnActorEnter(OtherActor);
	}
}

