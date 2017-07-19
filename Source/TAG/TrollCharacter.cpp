// Fill out your copyright notice in the Description page of Project Settings.

#include "TrollCharacter.h"

ATrollCharacter::ATrollCharacter() {

	//Create default shape
	InteractShape = CreateDefaultSubobject<USphereComponent>(FName("Interact Shape"));
	InteractShape->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);

	bReplicates = true;

}

/*
ATrollCharacter::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{

}
*/

void ATrollCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ATrollCharacter::Interact);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ATrollCharacter::StopInteract);

	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ATrollCharacter::StopInteract() {

	if (Role < ROLE_Authority) {
		ServerStopInteract();
	}
	else {
		bIsPunching = false;
	}
}

void ATrollCharacter::ServerStopInteract_Implementation()
{
	StopInteract();
}

bool ATrollCharacter::ServerStopInteract_Validate()
{
	return true;
}

void ATrollCharacter::Interact()
{
	if (Role < ROLE_Authority) {
		ServerInteract();
	}
	else {
		bPunchTimerStarted = true;
		bIsPunching = true;

		if (bPunchTimerStarted) {
			FTimerHandle TimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, this , &ATrollCharacter::DelayedInteract, 0.5f, false, 0);
		}

	}
}

void ATrollCharacter::DelayedInteract()
{
	SimulateInteractFX();

	TSubclassOf <class UDamageType> DamageTypeClass;
	const TArray<AActor*> IgnoreActors;

	UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, GetActorForwardVector() * 100.f + GetActorLocation(), DamageRadius, DamageTypeClass, IgnoreActors, this, GetController());

	bPunchTimerStarted = false;
}

void ATrollCharacter::ServerInteract_Implementation()
{
	Interact();
}

bool ATrollCharacter::ServerInteract_Validate()
{
	return true;
}

void ATrollCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATrollCharacter, bIsPunching);
}

void ATrollCharacter::OnRep_IsPunching()
{

}

void ATrollCharacter::SimulateInteractFX_Implementation()
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DamageParticles, GetActorForwardVector() * 100.f + GetActorLocation(), GetActorRotation(), true);
}
