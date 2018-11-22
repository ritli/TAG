// Fill out your copyright notice in the Description page of Project Settings.

#include "TrollCharacter.h"
#include "GnomeCharacter.h"
#include "InteractSceneComponent.h"
#include "TAGGameMode.h"

ATrollCharacter::ATrollCharacter() {

	//Create default shape
	InteractShape = CreateDefaultSubobject<USphereComponent>(FName("Interact Shape"));
	InteractShape->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);

	GnomePawn = AGnomeCharacter::StaticClass();

	bReplicates = true;
	AttackCount = 0;
	bHoldingAttack = false;
}

void ATrollCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CameraTick(DeltaSeconds);
}

void ATrollCharacter::CameraTick(float DeltaSeconds) {

	if (ActorToLookAt != nullptr) {
		FTransform T = GetFollowCamera()->GetComponentTransform();

		FVector Loc = FMath::Lerp(T.GetLocation(), ActorToLookAt->GetActorLocation(), CameraLookAtSpeed * DeltaSeconds);
		FRotator Rot = FMath::Lerp(T.Rotator(), ActorToLookAt->GetActorRotation(), CameraLookAtSpeed * DeltaSeconds);

		GetFollowCamera()->SetWorldLocationAndRotation(Loc, Rot);
		//GetFollowCamera()->SetWorldLocationAndRotation(ActorToLookAt->GetActorLocation(), ActorToLookAt->GetActorRotation());
	}
	else if (bResetCameraSlow) {
		
		//StopHoldAttack();
		StopAttack();

		FVector FromVector = GetFollowCamera()->RelativeLocation;
		FRotator FromRot = GetFollowCamera()->RelativeRotation;

		GetFollowCamera()->RelativeLocation = FMath::Lerp(FromVector, FVector::ZeroVector, CameraResetAlpha);
		GetFollowCamera()->RelativeRotation = FMath::Lerp(FromRot, FRotator::ZeroRotator, CameraResetAlpha);

		//GetFollowCamera()->RelativeLocation = FVector::ZeroVector;
		//GetFollowCamera()->RelativeRotation = FRotator::ZeroRotator;

		CameraResetAlpha += DeltaSeconds * CameraTransitionSpeed;

		if (CameraResetAlpha >= 1) {
			//if (GEngine)
			//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Troll Reset Slow"));
			GetFollowCamera()->RelativeLocation = FVector::ZeroVector;
			GetFollowCamera()->RelativeRotation = FRotator::ZeroRotator;
			CameraResetAlpha = 0;
			bResetCameraSlow = false;
		}
		
	}
	else if (bToogleMountCamera) {

		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("toggle"));

		//GetFollowCamera()->control = false;

		FVector FromVector = GetFollowCamera()->RelativeLocation;
		FRotator FromRot = GetFollowCamera()->RelativeRotation;

		GetFollowCamera()->RelativeLocation = FMath::Lerp(FromVector, FVector::ZeroVector, CameraResetAlpha);
		GetFollowCamera()->RelativeRotation = FMath::Lerp(FromRot, FRotator::ZeroRotator, CameraResetAlpha);

		CameraResetAlpha += (DeltaSeconds * CameraTransitionSpeed) / 5;

		if (CameraResetAlpha >= 1) {
			//if (GEngine)
			//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Troll Toggle"));
			GetFollowCamera()->RelativeLocation = FVector::ZeroVector;
			GetFollowCamera()->RelativeRotation = FRotator::ZeroRotator;
			CameraResetAlpha = 0;
			bToogleMountCamera = false;
			//GetFollowCamera()->bUsePawnControlRotation = true;
		}
	}
	else if (bResetCamera)
	{
		FVector FromVector = GetFollowCamera()->RelativeLocation;
		FRotator FromRot = GetFollowCamera()->RelativeRotation;

		//GetFollowCamera()->RelativeLocation = FMath::Lerp(FromVector, FVector::ZeroVector, CameraResetAlpha);
		//GetFollowCamera()->RelativeRotation = FMath::Lerp(FromRot, FRotator::ZeroRotator, CameraResetAlpha);

		GetFollowCamera()->RelativeLocation = FVector::ZeroVector;
		GetFollowCamera()->RelativeRotation = FRotator::ZeroRotator;

		//CameraResetAlpha += DeltaSeconds * CameraTransitionSpeed;
		CameraResetAlpha = 1;

		if (CameraResetAlpha >= 1) {
			//if (GEngine)
			//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Troll Reset"));
			CameraResetAlpha = 0;
			bResetCamera = false;
		}
	}
}

//Function called from gnome when mounting up
void ATrollCharacter::MountGnome()
{
	StopHoldAttack();
	StopAttack();

	OnMount();

	bIsMounting = true;
}

void ATrollCharacter::FinishMountGnome() {

	if (SpawnedPawn) {
		SpawnedPawn->Destroy();

		SpawnedPawn = nullptr;
		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(nullptr);
	}

	ChangeState(EPlayerType::Troll);

}

float ATrollCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= Damage;

	if (CurrentHealth <= 0) {
		OnDeath();
		//ServerResetPlayer(Controller);
	}

	return CurrentHealth;
}

void ATrollCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Throw/Pet", IE_Pressed, this, &ATrollCharacter::Attack);
	PlayerInputComponent->BindAction("Throw/Pet", IE_Released, this, &ATrollCharacter::StopAttack);

	//PlayerInputComponent->BindAction("HoldAttack", IE_Pressed, this, &ATrollCharacter::HoldAttack);
	//PlayerInputComponent->BindAction("HoldAttack", IE_Released, this, &ATrollCharacter::StopHoldAttack);

	//PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &ATrollCharacter::HoldThrow);
	//PlayerInputComponent->BindAction("Throw", IE_Released, this, &ATrollCharacter::StopHoldThrow);

	//PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ATrollCharacter::Interact);

	PlayerInputComponent->BindAction("Switch Character", IE_Pressed, this, &ATrollCharacter::ToggleState);

	PlayerInputComponent->BindAction("Mount", IE_Pressed, this, &ATrollCharacter::MountState);

	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATrollCharacter::HoldThrow() {
	if (bCanThrow) {
		//bDisplayThrowArc = true;
	}
}

void ATrollCharacter::StopHoldThrow() {
	if (bCanThrow) {

		bDisplayThrowArc = false;

		OnThrow();
	}
}

void ATrollCharacter::HoldAttack() {
	//bHoldingAttack = true;
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

		CameraTransitionSpeed = 1.25f; //1.25

		AGnomeCharacter* GnomeCharacter = Cast<AGnomeCharacter>(SpawnedPawn);

		float distance = FVector::Dist(SpawnedPawn->GetActorLocation(), GetActorLocation());

		/*
		if (distance < GnomeCharacter->GetMountDistance()) {
			MountGnome();

			return;
		}
		*/

		StopHoldAttack();
		StopAttack();

		//Cast<AGnomeCharacter>(SpawnedPawn)->GetFollowCamera()->SetWorldLocationAndRotation(GetFollowCamera()->GetComponentLocation(), GetFollowCamera()->GetComponentRotation());

		//Cast<AGnomeCharacter>(SpawnedPawn)->ResetCamera();
		Cast<AGnomeCharacter>(SpawnedPawn)->ResetCameraSlow();
		Controller->Possess(Cast<APawn>(SpawnedPawn));

		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(Cast<AGnomeCharacter>(SpawnedPawn));
		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentPlayerType(EPlayerType::Gnome);

		return;
	}

	/*
	//Variables for spawning, need to be declared out side of switch
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = this;
	SpawnParameters.Owner = GetController();

	FQuat rotation = GetActorRotation().Quaternion();
	FVector offset = rotation * DismountOffset;

	switch (toState)
	{
	case EPlayerType::Troll:
		

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
	*/
}

void ATrollCharacter::ToggleState() {
	if (!bIsMounting)
	{
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
}

void ATrollCharacter::MountState()
{
	Mount(EPlayerType::Gnome);

	/*
	switch (CurrentState)
	{
	case EPlayerType::Troll:
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("troll -> gnome"));
		Mount(EPlayerType::Gnome);
		break;
	case EPlayerType::Gnome:
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("gnome -> troll"));
		Mount(EPlayerType::Troll);
		break;
	default:
		break;
	}
	*/
}

void ATrollCharacter::Mount(PlayerType toState)
{
	CurrentState = toState;

	if (SpawnedPawn && !bIsMounting) {
	
		AGnomeCharacter* GnomeCharacter = Cast<AGnomeCharacter>(SpawnedPawn);

		float distance = FVector::Dist(SpawnedPawn->GetActorLocation(), GetActorLocation());

		if (distance < GnomeCharacter->GetMountDistance()) {
			MountGnome();

			return;
		}
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

		Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentTroll(this);
		break;
	case EPlayerType::Gnome:
		OnDismount();
		break;
	default:
		break;
	}

	Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentPlayerType(toState);
	bIsMounting = false;
}

void ATrollCharacter::FinishDismount(FVector Location) {
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = this;
	SpawnParameters.Owner = GetController();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->GetGnomeCharacter() == nullptr) {
		SpawnedPawn = GetWorld()->SpawnActor<AGnomeCharacter>(GnomePawn, Location,  GetActorRotation() , SpawnParameters);
	}

	Cast<AGnomeCharacter>(SpawnedPawn)->GetFollowCamera()->SetWorldLocationAndRotation(GetFollowCamera()->GetComponentLocation(), GetFollowCamera()->GetComponentRotation());
	Cast<AGnomeCharacter>(SpawnedPawn)->SetTrollParent(this);
	CameraTransitionSpeed = 1.25; // 0.33

	Cast<AGnomeCharacter>(SpawnedPawn)->ToggleMountCamera();
	//Cast<AGnomeCharacter>(SpawnedPawn)->ResetCamera();
	Controller->Possess(Cast<APawn>(SpawnedPawn));

	Cast<ATAGGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetCurrentGnome(Cast<AGnomeCharacter>(SpawnedPawn));

}
