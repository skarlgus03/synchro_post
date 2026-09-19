#include "Unit/UnitHealthBarComponent.h"
#include "UI/UnitHealthBarWidget.h"
#include "Unit/Unit.h"
#include "Unit/UnitDataAsset.h"
#include "Unit/StatComponent.h"
#include "Framework/SynchroPostSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

UUnitHealthBarComponent::UUnitHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(120.0f, 28.0f));

	// 위젯 컴포넌트는 기본적으로 충돌 박스를 만든다.
	// 유닛 클릭/호버 레이캐스트를 체력바가 가로채지 않도록 끈다.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	// 유닛 데이터가 정해지기 전에는 보이지 않는다.
	SetVisibility(false);
}

void UUnitHealthBarComponent::Refresh(AUnit* InOwnerUnit)
{
	if (InOwnerUnit)
	{
		CachedOwnerUnit = InOwnerUnit;
	}

	const UUnitDataAsset* UnitData = GetUnitData();
	if (!UnitData)
	{
		SetVisibility(false);
		return;
	}

	// (1) 높이 — 메시가 세팅된 뒤에만 의미가 있다
	SetRelativeLocation(FVector(0.0f, 0.0f, CalculateHeight()));

	// (2) 위젯 클래스
	TSubclassOf<UUnitHealthBarWidget> WidgetClassToUse = UnitData->HealthBarWidgetClass;
	if (!WidgetClassToUse)
	{
		if (const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>())
		{
			WidgetClassToUse = Settings->DefaultHealthBarWidgetClass;
		}
	}
	if (!WidgetClassToUse)
	{
		WidgetClassToUse = UUnitHealthBarWidget::StaticClass();
	}

	// 같은 클래스로 다시 세팅하면 위젯이 재생성될 수 있으므로 달라졌을 때만
	if (GetWidgetClass() != WidgetClassToUse)
	{
		SetWidgetClass(WidgetClassToUse);
	}

	// (3) 초기값
	UUnitHealthBarWidget* BarWidget = GetBarWidget();
	if (!BarWidget)
	{
		// 클라에서 OnRep_UnitData가 위젯 생성보다 먼저 오는 경우.
		// AUnit::BeginPlay에서 다시 호출되므로 여기선 숨긴 채 반환한다.
		// (실패가 아니라 '아직 이른 것'이라 로그를 남기지 않는다)
		SetVisibility(false);
		return;
	}

	const UStatComponent* Stat = GetOwnerUnit() ? GetOwnerUnit()->GetStatComponent() : nullptr;
	const int32 InitialHealth = Stat ? Stat->GetCurrentHealth() : 0;
	const int32 InitialMaxHealth = Stat ? Stat->GetStat(SPTags::Stat::Combat::Primary::MaxHealth) : 0;
	BarWidget->InitializeHealthBar(InitialHealth, InitialMaxHealth);

	// 초기값이 들어간 뒤에 첫 판정
	UpdateVisibility();
}

void UUnitHealthBarComponent::NotifyHealthPresented(int32 DisplayAmount, int32 NewTargetHealth,
	bool bIsCritical, const FGameplayTagContainer& TypeTags)
{
	UUnitHealthBarWidget* BarWidget = GetBarWidget();
	if (!BarWidget)
	{
		return;
	}

	// 게이지를 먼저 갱신해야 UpdateVisibility가 새 값으로 판정한다.
	BarWidget->AnimateToHealth(NewTargetHealth);
	BarWidget->ShowDamageNumber(DisplayAmount, bIsCritical, TypeTags);

	// 피해든 회복이든, 연출이 끝날 때까지 바를 붙잡아 둔다.
	// (회복으로 풀피가 되는 순간 바가 증발해서 '+50'을 못 보는 걸 막는다)
	SetReason(EHealthBarReason::RecentHit, true);

	if (UWorld* World = GetWorld())
	{
		// 같은 핸들로 다시 걸면 갱신된다 — 연타로 맞아도 마지막 피격 기준으로 밀린다
		World->GetTimerManager().SetTimer(
			RecentHitTimerHandle, this, &UUnitHealthBarComponent::ClearRecentHit,
			RecentHitHoldSeconds, false);
	}

	// RecentHit이 이미 켜져 있었다면 SetReason이 갱신을 건너뛰므로,
	// 체력 변화를 반영하기 위해 여기서 한 번 더 부른다.
	UpdateVisibility();
}

void UUnitHealthBarComponent::SetHovered(bool bHovered)
{
	SetReason(EHealthBarReason::Hovered, bHovered);
}

void UUnitHealthBarComponent::SetSelected(bool bSelected)
{
	SetReason(EHealthBarReason::Selected, bSelected);
}

void UUnitHealthBarComponent::SetReason(EHealthBarReason Reason, bool bEnable)
{
	const EHealthBarReason Previous = Reasons;

	if (bEnable)
	{
		Reasons |= Reason;
	}
	else
	{
		Reasons &= ~Reason;
	}

	if (Previous != Reasons)
	{
		UpdateVisibility();
	}
}

void UUnitHealthBarComponent::ClearRecentHit()
{
	SetReason(EHealthBarReason::RecentHit, false);
}

void UUnitHealthBarComponent::UpdateVisibility()
{
	const UUnitDataAsset* UnitData = GetUnitData();

	const EHealthBarDisplay Policy = UnitData
		? UnitData->HealthBarDisplay
		: EHealthBarDisplay::WhenDamaged;

	// Disabled는 사건 이유까지 전부 무시한다. (보스: 전용 UI로 대체)
	if (Policy == EHealthBarDisplay::Disabled)
	{
		SetVisibility(false);
		return;
	}

	// 상시 조건은 저장하지 않고 매번 계산한다.
	// 기준은 논리 체력이 아니라 '연출 체력'이다 — 서버는 스킬을 0.1초 안에 해결하지만
	// 연출은 큐가 재생할 때 일어나므로, StatComponent를 보면 칼을 휘두르기도 전에 바가 반응한다.
	bool bIdleVisible = false;

	switch (Policy)
	{
	case EHealthBarDisplay::Always:
		bIdleVisible = true;
		break;

	case EHealthBarDisplay::WhenDamaged:
		if (const UUnitHealthBarWidget* BarWidget = GetBarWidget())
		{
			bIdleVisible = !BarWidget->IsAtFullHealth();
		}
		break;

	case EHealthBarDisplay::EventOnly:
	default:
		bIdleVisible = false;
		break;
	}

	SetVisibility(bIdleVisible || Reasons != EHealthBarReason::None);
}

float UUnitHealthBarComponent::CalculateHeight() const
{
	const AUnit* OwnerUnit = GetOwnerUnit();
	const UUnitDataAsset* UnitData = GetUnitData();
	USkeletalMeshComponent* Mesh = OwnerUnit ? OwnerUnit->GetMesh() : nullptr;

	if (!Mesh || !UnitData)
	{
		return FallbackHeight;
	}

	const float ZOffset = UnitData->HealthBarZOffset;

	// 1순위: DA가 지정한 본/소켓. 실제로 존재할 때만 쓴다.
	const FName SocketName = UnitData->HealthBarSocket;
	if (!SocketName.IsNone())
	{
		if (Mesh->DoesSocketExist(SocketName))
		{
			return Mesh->GetSocketTransform(SocketName, RTS_Component).GetLocation().Z + ZOffset;
		}

		// 지정했는데 없다 = DA 설정 실수. 조용히 넘기지 않는다.
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] HealthBarSocket '%s'이(가) 메시에 없음. 바운즈로 대체한다."),
			*GetNameSafe(OwnerUnit), *SocketName.ToString());
	}

	// 2순위: 메시 애셋의 레퍼런스 포즈 바운즈 최상단.
	// 애셋에 미리 저장된 값이라 애님 인스턴스 평가 여부와 무관하게 안전하다.
	if (const USkeletalMesh* MeshAsset = Mesh->GetSkeletalMeshAsset())
	{
		const FBoxSphereBounds MeshBounds = MeshAsset->GetBounds();
		return MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z + ZOffset;
	}

	return FallbackHeight;
}

UUnitHealthBarWidget* UUnitHealthBarComponent::GetBarWidget() const
{
	return Cast<UUnitHealthBarWidget>(GetUserWidgetObject());
}

const UUnitDataAsset* UUnitHealthBarComponent::GetUnitData() const
{
	const AUnit* OwnerUnit = GetOwnerUnit();
	return OwnerUnit ? OwnerUnit->CurrentUnitData.Get() : nullptr;
}