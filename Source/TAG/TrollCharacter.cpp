// Fill out your copyright notice in the Description page of Project Settings.

#include "TrollCharacter.h"
#include "GnomeCharacter.h"
#include "InteractSceneComponent.h"
#include "TAGGameMode.h"

ATrollCharacter::ATrollCharacter() {

	//Create default shape
	InteractShape = CreateDefaultSubobject<USphereComponent>(FName("Interact Shape"));
	InteractShape->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);

	bReplicates = true;
	AttackCount = 0;
	bHoldingAttack = false;
}

void ATrollCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bResetCamera) {
		FVector FromVector = GetFollowCamera()->RelativeLocation;
		FRotator FromRot = GetFollowCamera()->RelativeRotation;

		GetFollowCamera()->RelativeLocation = FMath::Lerp(FromVector, FVector::ZeroVector, CameraResetAlpha);
		GetFollowCamera()->RelativeRotation = FMath::Lerp(FromRot, FRotator::ZeroRotator, CameraResetAlpha);

		CameraResetAlpha += DeltaSeconds * 1.25f;

		if (CameraResetAlpha >= 1) {
			bResetCamera = false;
		}
	}
}

//Function called from gnome when mounting up
void ATrollCharacter::MountGnome()
{
	bIsMounting = true;

	if (SpawnedPawn){
		SpawnedPawn->Destroy();

		SpawnedPawn = nullptr;
		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(nullptr);
	}

	ChangeState(EPlayerType::Troll);
}

void ATrollCharacter::ResetCamera()
{
	bResetCamera = true;
	CameraResetAlpha = 0;
}

float ATrollCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= Damage;

	if (CurrentHealth <= 0) {
		OnDeath();
		ServerResetPlayer(Controller);
	}

	return CurrentHealth;
}

void ATrollCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ATrollCharacter::Attack);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &ATrollCharacter::StopAttack);

	PlayerInputComponent->BindAction("HoldAttack", IE_Pressed, this, &ATrollCharacter::HoldAttack);
	PlayerInputComponent->BindAction("HoldAttack", IE_Released, this, &ATrollCharacter::StopHoldAttack);


	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ATrollCharacter::Interact);

	PlayerInputComponent->BindAction("SwitchState", IE_Pressed, this, &ATrollCharacter::ToggleState);

	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATrollCharacter::HoldAttack() {
	bHoldingAttack = true;
}

void ATrollCharacter::StopHoldAttack()
{
	bHoldingAttack = false;
}

void ATrollCharacter::StopAttack() {

	bIsPunching = false;
}

void ATrollCharacter::Attack()
{
	bPunchTimerStarted = true;

	if (!bIsPunching) {
		OnAttack();
	}

	bIsPunching = true;
}

void ATrollCharacter::DelayedAttack()
{
// 	TSubclassOf <class UDamageType> DamageTypeClass;
// 	const TArray<AActor*> IgnoreActors;
// 
// 	UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, GetActorForwardVector() * 100.f + GetActorLocation(), DamageRadius, DamageTypeClass, IgnoreActors, this, GetController());

	bPunchTimerStarted = false;
}

void ATrollCharacter::Interact()
{
	TArray<AActor*> OutActors;

	InteractShape->GetOverlappingActors(OutActors);

	int8 size = OutActors.Num();

	for (int8 i = 0; i < size; i++)
	{
		if (OutActors[i]->GetComponentByClass(UInteractSceneComponent::StaticClass())) {
			Cast<UInteractSceneComponent>(OutActors[i]->GetComponentByClass(UInteractSceneComponent::StaticClass()))->Interact(this);
		}
	}
}

void ATrollCharacter::DealDamage() {
	AttackCount++;
// 		TSubclassOf <class UDamageType> DamageTypeClass;
// 		const TArray<AActor*> IgnoreActors;
//
//		UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, GetActorForwardVector() * 100.f + GetActorLocation(), DamageRadius, DamageTypeClass, IgnoreActors, this, GetController());

	if (AttackCount >= 2) {
		StopAttack();
	}
}

void ATrollCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void ATrollCharacter::ChangeState(PlayerType toState)
{
	CurrentState = toState;

	//Check if gnome is already spawned, if it is then a character switch will occur
	if (SpawnedPawn && !bIsMounting) {
		AGnomeCharacter* GnomeCharacter = Cast<AGnomeCharacter>(SpawnedPawn);

		Cast<AGnomeCharacter>(SpawnedPawn)->GetFollowCamera()->SetWorldLocationAndRotation(GetFollowCamera()->GetComponentLocation(), GetFollowCamera()->GetComponentRotation());
		Cast<AGnomeCharacter>(SpawnedPawn)->ResetCamera();
		Controller->Possess(Cast<APawn>(SpawnedPawn));

		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(Cast<AGnomeCharacter>(SpawnedPawn));
		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentPlayerType(EPlayerType::Gnome);

		return;
	}

	//Variables for spawning, need to be declared out side of switch
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = this;
	SpawnParameters.Owner = GetController();

	FQuat rotation = GetActorRotation().Quaternion();
	FVector offset = rotation * DismountOffset;

	switch (toState)
	{
	case EPlayerType::Troll:
		OnMount();

		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentTroll(this);
		break;
	case EPlayerType::Gnome:
// 		SpawnedPawn = GetWorld()->SpawnActor<AGnomeCharacter>(GnomePawn, GetActorLocation() + offset, GetActorRotation(), SpawnParameters);
// 
// 		Cast<AGnomeCharacter>(SpawnedPawn)->GetFollowCamera()->SetWorldLocationAndRotation(GetFollowCamera()->GetComponentLocation(), GetFollowCamera()->GetComponentRotation());
// 		Cast<AGnomeCharacter>(SpawnedPawn)->SetTrollParent(this);
// 		Cast<AGnomeCharacter>(SpawnedPawn)->ResetCamera();
// 		Controller->Possess(Cast<APawn>(SpawnedPawn));
// 
// 		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(Cast<AGnomeCharacter>(SpawnedPawn));

		OnDismount();
		break;
	default: 
		break;
	}

	Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentPlayerType(toState);
	bIsMounting = false;
}

void ATrollCharacter::ToggleState() {
	switch (CurrentState)
	{
	case EPlayerType::Troll:
		ChangeState(EPlayerType::Gnome);
		break;
	case EPlayerType::Gnome:
		ChangeState(EPlayerType::Troll);

		break;
	default:
		break;
	}
}

void ATrollCharacter::FinishDismount(FVector Location) {
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = this;
	SpawnParameters.Owner = GetController();

	SpawnedPawn = GetWorld()->SpawnActor<AGnomeCharacter>(GnomePawn, Location, GetActorRotation(), SpawnParameters);

	Cast<AGnomeCharacter>(SpawnedPawn)->GetFollowCamera()->SetWorldLocationAndRotation(GetFollowCamera()->GetComponentLocation(), GetFollowCamera()->GetComponentRotation());
	Cast<AGnomeCharacter>(SpawnedPawn)->SetTrollParent(this);
	Cast<AGnomeCharacter>(SpawnedPawn)->ResetCamera();
	Controller->Possess(Cast<APawn>(SpawnedPawn));

	Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(Cast<AGnomeCharacter>(SpawnedPawn));

}
