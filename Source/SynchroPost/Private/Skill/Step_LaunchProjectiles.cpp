#include "Skill/Step_LaunchProjectiles.h"
#include "Skill/ProjectileActor.h"
#include "Unit/Unit.h"
#include "Framework/GridManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SynchroPost.h"

void UStep_LaunchProjectiles::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;

	// 캐스터와 투사체 클래스가 없으면 발사할 수 없다. (발사 실패로 간주)
	UWorld* World = GetContextWorld();
	if (!World || !Ctx.Caster.IsValid() || !ProjectileClass)
	{
		UE_LOG(LogSP, Verbose, TEXT("[SP] 투사체 발사: 캐스터 또는 투사체 클래스 없음"));
		Finish();
		return;
	}

	// 타겟이 없으면 발사할 필요가 없다. (발사 실패로 간주)
	MyTargets = Ctx.Payload.Targets;
	if (MyTargets.IsEmpty())
	{
		Finish();
		return;
	}

	// 발사할 투사체 수를 카운트하고, 발사 시작
	PendingArrivals = MyTargets.Num();
	UE_LOG(LogSP, Verbose, TEXT("[SP] 투사체 %d발 발사"), PendingArrivals);

	// IntervalBetweenShots이 0이면 동시 발사, 0보다 크면 순차 발사
	for (int32 i = 0; i < MyTargets.Num(); ++i)
	{
		if (IntervalBetweenShots <= 0.f || i == 0)
		{
			LaunchOne(i);
			continue;
		}

		FTimerHandle& Handle = ShotTimers.AddDefaulted_GetRef();
		World->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &UStep_LaunchProjectiles::LaunchOne, i),
			IntervalBetweenShots * i, false);
	}

	if (!bWaitForAllArrivals)
	{
		Finish();
	}
}

void UStep_LaunchProjectiles::LaunchOne(int32 Index)
{
	// Index가 유효하지 않으면 발사할 수 없다. (발사 실패로 간주)
	if (!MyTargets.IsValidIndex(Index))
	{
		return;
	}

	AUnit* Caster = Ctx.Caster.Get();
	UWorld* World = GetContextWorld();
	UGridManager* GridManager = World ? World->GetSubsystem<UGridManager>() : nullptr;

	// 발사 실패해도 결과는 표시하고 카운트를 깎는다. 안 그러면 대본이 멈춘다.
	if (!Caster || !World || !GridManager)
	{
		UE_LOG(LogSP, Warning, TEXT("[SP] 투사체 발사 실패 - 즉시 결과 표시로 대체"));
		HandleArrival(MyTargets[Index]);
		return;
	}
	
	// 발사 위치 결정
	USkeletalMeshComponent* MeshComp = Caster->GetMesh();

	FVector SpawnLocation = Caster->GetActorLocation();
	FVector MuzzleLocation;
	if (Caster->FindSocketLocation(MuzzleSocketName, MuzzleLocation))
	{
		SpawnLocation = MuzzleLocation;
	}

	const AActor* TargetActor = MyTargets[Index].Target.Get();
	const FVector Destination = TargetActor
		? TargetActor->GetActorLocation() + FVector(0.f, 0.f, 60.f)   // 가슴 높이
		: GridManager->GetTileWorldLocation(MyTargets[Index].Coordinate);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Caster;
	
	// 투사체 생성
	AProjectileActor* Projectile = World->SpawnActor<AProjectileActor>(
		ProjectileClass, SpawnLocation, (Destination - SpawnLocation).Rotation(), SpawnParams);

	if (!Projectile)
	{
		HandleArrival(MyTargets[Index]);
		return;
	}

	// 구독을 먼저, Initialize(타이머 시작)를 나중에.
	Projectile->OnArrived.AddUObject(this, &UStep_LaunchProjectiles::HandleArrival);
	Projectile->Initialize(MyTargets[Index], Destination, ImpactEffect, Caster, ProjectileSpeed);
}

void UStep_LaunchProjectiles::HandleArrival(const FCombatEventTarget& ArrivedResult)
{
	// Abort 이후에 뒤늦게 도착한 투사체는 무시
	if (IsFinished())
	{
		return;
	}

	// 이 투사체가 담당한 타겟만 표시한다.
	// 거리가 달라 도착 시각이 제각각이어도 각자 자기 시점에 터진다.
	PresentTargetResult(ArrivedResult);

	--PendingArrivals;
	UE_LOG(LogSP, Verbose, TEXT("[SP] 투사체 도착 (남은 %d)"), PendingArrivals);

	if (PendingArrivals <= 0 && bWaitForAllArrivals)
	{
		Finish();
	}
}

void UStep_LaunchProjectiles::Abort()
{
	if (UWorld* World = GetContextWorld())
	{
		for (FTimerHandle& Handle : ShotTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
	ShotTimers.Reset();
}