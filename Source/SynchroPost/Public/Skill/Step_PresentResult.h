#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillStep.h"
#include "GameplayTagContainer.h"
#include "Step_PresentResult.generated.h"

class USkillComponent;

/* 큐 한 줄기: 이 큐가 올 때마다, 맡은 피해 조각들을 한 토막씩 보여준다 */
USTRUCT()
struct FResultTrack
{
	GENERATED_BODY()

	// 이 태그와 정확히 같은 큐만 받는다
	UPROPERTY(EditAnywhere, Category = "Track", meta = (Categories = "Skill.Cue"))
	FGameplayTag CueTag;

	// 맡은 조각 하나를 몇 토막으로 나누나 = 이 큐가 몇번 오는가
	UPROPERTY(EditAnywhere, Category = "Track", meta = (ClampMin = "1"))
	int32 CueCount = 1;

	// 맡을 조각의 태그  (예: Trait.DamageType.Fire).
	// 비우면 남은 조각 전부. 위 트랙부터 가져감.
	UPROPERTY(EditAnywhere, Category = "Track")
	FGameplayTagContainer HitFilter;
};


/*
* 서버 결과를 화면에 표시한다 (체력바/ 숫자/ 피격반응). 표시 단위는 피해 조각
* 트랙이 없으면 모든 조각을 즉시 표시하고 끝
* 트랙이 있으면 큐에 맞춰 나눠 표시하고, 모든 트랙이 끝나면 끝난다.
* 큐가 덜 와도 MaxWaitSeconds가 지나면 남은 토막을 몰아서 표시함
* 숫자는 계산된 값 그대로, 체력바는 서버의 체력 전~후를 기준으로 표시. 체력바는 서버가 보내준 값과 다를 수 있음
*/
UCLASS(DisplayName = "결과 표시")
class SYNCHROPOST_API UStep_PresentResult : public USkillStep
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Result")
	TArray<FResultTrack> Tracks;

	// 큐가 다 안 와도 이 시간 지나면 남은 토막을 몰아서 표시하고 끝낸다 최소 0.1초. 
	UPROPERTY(EditAnywhere, Category = "Result", meta = (ClampMin = "0.1"))
	float MaxWaitSeconds = 5.f;

	virtual void Start(const FSkillPresentationContext& InCtx) override;
	virtual void Abort() override;

private:
	struct FPiecePlay
	{
		int32 TargetIndex = INDEX_NONE;   // Ctx.Payload.Targets 인덱스
		int32 Signed = 0;                 // 표시할 양 (피해 음수, 회복 양수)
		FGameplayTagContainer Tags;       // 조각 태그 (숫자 색 / 트랙 필터)
		bool bCritical = false;
		int32 Shown = 0;                  // 지금까지 보여준 양
	};

	struct FTrackPlay
	{
		TArray<int32> PieceIndices;       // Pieces 인덱스
		int32 Received = 0;
	};

	/*
	* 서버 결과 (Targets -> Hits -> Parts) 를 펼쳐서 Pieces 를 만든다.
	* 그리고 조각마다 맡을 트랙을 정하고, 위 트랙부터 필터가 맞는 걸 가져가고
	* 아무도 안 가져간 조각은 나중에 몰아서 표시한다.
	* 대상별 체력값 RawHealth 도 여기서 채운다.
	*/
	void BuildPlan();
	// 더 기다릴 큐가 남았는지 확인한다.
	bool AllTracksDone() const;

	/*
	* 애니메이션 노티파이 큐가 올때마다 불린다. 태그가 맞는 트랙을 찾아서
	* 그 트랙이 맡은 조각들의 이번 토막을 보여주고 받은 횟수를 올린다.
	*/
	void HandleSkillCue(FGameplayTag CueTag);
	/*
	* 큐가 MaxWaithSeconds 안에 다 오지 않았을 떄 타이머가 부른다.
	* 경고를 남기고 남은 걸 몰아서 보여주고 끝냄
	*/
	void HandleTimeout();

	/*
	* 숫자 하나를 띄우는 유일한 함수
	* 누적 체력을 갱신하고 체력바 값은 서버의 전~후 범위로 잘라서 ApplyVisualDamage를 부른다.
	*/
	void ShowAmount(FPiecePlay& Piece, int32 Amount);
	/*
	* 아직 덜 보여준 조각들의 남은 양을 한 번에 보여준다.
	* 정상 종료, 타임아웃, Abort 어느 경로로 끝나든 체력바가 서버 값으로 맞춰지게 한다.
	* bWarn은 왜 몰아서 보여주는지 경고를 남긴다.
	*/
	void FlushRemaining(bool bWarn);

	// 정상적으로 끝낼 때 불러짐. Unbind()만 하고 Finish()로 다음 Step에 넘김
	void Complete();
	// 큐 구독을 해제하고 타이머를 지움
	void Unbind();

	TArray<FPiecePlay> Pieces;
	TArray<FTrackPlay> TrackPlays;
	TArray<int32> RawHealth;              // 대상별, 자르기 전 누적 체력

	FDelegateHandle CueHandle;
	TWeakObjectPtr<USkillComponent> BoundSkillComp;
	FTimerHandle TimeoutHandle;
};