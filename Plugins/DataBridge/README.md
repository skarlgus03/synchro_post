# DataBridge v1.0

언리얼 에셋(DataAsset 등)을 JSON으로 꺼내고, AI나 사람이 고친 JSON을 **안전하게** 다시 넣는 에디터 플러그인.

- 에셋 → JSON (Export), 클래스 → 설명서 (Describe), JSON → 에셋 (DryRun / Import)
- 적용 전에 사본으로 연습(Rehearse)하고, 실제 적용은 Undo 한 번으로 되돌릴 수 있다
- 틀린 값은 **정확한 경로와 쓸 수 있는 값 목록**과 함께 거부한다
- 게임 모듈에 의존하지 않는다 (UE 5.7, Editor 모듈)

## 콘솔 명령

| 명령 | 하는 일 | 에셋 변경 |
|---|---|---|
| `DataBridge.Export <에셋 경로...>` | 에셋을 JSON으로. `Saved/DataBridge/Export.json` | 없음 |
| `DataBridge.Describe <클래스> [콘텐츠 폴더...]` | 클래스 설명서 + 카탈로그. `Saved/DataBridge/Describe_<클래스>.json` | 없음 |
| `DataBridge.DryRun <파일>` | 계획 → 사본에 연습 → 변경 목록 출력 | 없음 |
| `DataBridge.Import <파일>` | 계획 → 연습 → 실제 적용 (Undo 기록 1개, 저장 안 함) | 있음 |

- 에셋 경로는 `/Game/A/DA_X` 또는 콘텐츠 브라우저의 Copy Reference 형식 모두 받는다.
- 파일 경로가 상대 경로면 프로젝트 폴더 기준.
- 클래스는 전체 경로(`/Script/SynchroPost.SkillDataAsset`) 또는 짧은 이름(`SkillDataAsset`).
- Describe의 콘텐츠 폴더를 생략하면 `/Game` 전체. 예: `DataBridge.Describe SkillDataAsset /Game/SynchroPost /Game/Niagara`

## AI로 데이터 만들기

1. `DataBridge.Describe <클래스> <폴더...>`로 설명서를 만든다 (C++ 클래스가 바뀔 때만 다시).
2. 비슷한 기존 에셋을 `DataBridge.Export`로 예시로 꺼낸다.
3. 새 AI 대화에 설명서 + 예시를 주고 요청한다. "설명서에 없는 태그/에셋/클래스는 지어내지 말 것"을 명시한다.
   쓸 몽타주·이펙트가 정해져 있으면 요청에 적는다 (카탈로그에는 이름만 있어 용도는 모른다).
4. 결과를 파일로 저장하고 `DataBridge.DryRun`. 실패하면 Issues 로그를 그대로 AI에게 돌려준다.
5. 통과하면 `DataBridge.Import` → 에디터/PIE에서 확인 → 저장하거나 Ctrl+Z.

## 문서 형식

```json
{
  "format": "databridge/1",
  "assets": [
    { "path": "/Game/Folder/Asset", "class": "/Script/Module.Class", "data": { "필드": 값 } }
  ]
}
```

| 타입 | JSON 표기 |
|---|---|
| bool / 숫자 / 문자열 / FName / FText | 그대로 (FText는 원문) |
| enum | 이름 `"WhenDamaged"` (`"EEnum::WhenDamaged"`도 허용) |
| 비트마스크 (`Bitmask`, `BitmaskEnum` 메타) | 이름 배열 `["Enemy", "Obstacle"]` (숫자도 허용) |
| FGameplayTag / Container | `"A.B.C"` / `["A.B", "C.D"]` |
| FTransform | `{ "Location": {X,Y,Z}, "Rotation": {Pitch,Yaw,Roll}, "Scale": {X,Y,Z} }` (도 단위, 축 생략 가능) |
| 구조체 | 필드 객체 |
| 배열 / Set | 배열 (통째로 교체, Set 중복은 에러) |
| 맵 | 키가 문자열 계열이면 `{ "키": 값 }`, 아니면 `[ { "key": k, "value": v } ]` |
| 에셋 / 클래스 참조 | 경로 문자열 또는 `null` (BP 클래스는 `_C`로 끝남) |
| Instanced 서브오브젝트 | `{ "$class": "/Script/Module.Class", 필드... }` 또는 `null` |

## Import 규칙

- JSON에 **있는 필드만** 바꾼다. 생략한 필드는 기존 값 유지 (새 에셋은 클래스 기본값).
- 모르는 필드, 편집할 수 없는 필드(`EditAnywhere`가 아니거나 Transient/VisibleAnywhere)는 **에러**.
- 배열·Set·맵은 통째로 교체. Instanced 블록은 항상 새 객체 (생략 필드 = 클래스 기본값).
- `ClampMin`/`ClampMax` 위반은 잘라주지 않고 에러. 정수는 타입 범위도 검사.
- 존재하지 않는 태그·에셋·클래스는 에러.
- 에셋의 `IsDataValid`(const 버전)도 연습 단계에서 검사한다.

## 안전장치

- **Rehearse**: 실제 에셋의 사본(또는 새 객체)에 똑같이 적용·검증한 뒤 버린다. 실패하면 실제 에셋은 건드리지 않는다.
- **Commit**: 에디터 트랜잭션 하나("DataBridge Import")로 묶는다. 저장하지 않는다 (dirty만).
  - 도중에 실패하면 **자기 기록일 때만** Undo하고(트랜잭션 ID 비교) 새로 만든 에셋은 지운다.
  - 성공 후 Ctrl+Z 한 번이면 기존 에셋의 변경과 dirty 상태가 되돌아간다.
  - **새로 만든 에셋은 Undo 대상이 아니다** (에디터의 에셋 생성과 같은 규칙). 필요하면 직접 삭제.

## 설명서 (Describe)

- `classes` / `structs`: 정의 테이블. 필드별 `type`, `description`(C++ 주석), `values`(enum/flags), `allowed`(Instanced에 넣을 수 있는 클래스), `min`/`max`, `categories`(태그 접두사).
- `defaults`: 클래스 기본값 (생략하면 들어가는 값).
- `undocumented`: 주석 없는 필드 목록. 단위나 의미가 애매한 필드부터 주석을 채우면 AI 정확도가 오른다.
- `catalog`: 실제로 존재하는 값.
  - `tags`: 직접 등록한 태그와 Comment
  - `assets`: 에셋 필드의 클래스별 실제 에셋 경로 (AssetRegistry, 지정 폴더 안)
  - `classes`: 클래스 필드의 후보 (C++ 클래스 + 로드되지 않은 BP 포함)
  - 목록당 300개 상한. `UObject`처럼 너무 넓은 필드는 생략.

## 확장

| 확장 지점 | 언제 |
|---|---|
| `IDataBridgeFormat` | JSON 외의 파일 형식 (예: CSV) |
| `IDataBridgeHandler` | 필드 나열로는 엉망이 되는 에셋 (예: Material Instance). Export / Import / Describe를 함께 구현 |
| `IDataBridgeValueFormatter` | 특정 구조체를 읽기 쉬운 표기로 (예: 태그, 트랜스폼). Export / Import / DescribeValue를 함께 구현 |

구현체는 `IModularFeature`로 등록한다 (`FDataBridgeRegistry::Register`). 핸들러는 대상 클래스와 상속 거리가 가장 가까운 것이 선택된다.

## 테스트

에디터 콘솔에서 `Automation RunTests DataBridge` (23개).
결과는 `Saved/Logs/<프로젝트>.log`의 마지막 `Automation Test Queue Empty N tests performed.`와 `Result={Fail` 검색으로 확인.

## 알려진 한계

- Weak / Lazy 포인터, 델리게이트, 인터페이스 프로퍼티는 다루지 않는다 (설명서에서도 빠짐).
- 배열·맵 원소에 붙은 비트마스크와 Clamp는 적용되지 않는다 (필드에 직접 붙은 것만).
- Instanced `allowed`는 C++ 클래스와 로드된 BP만 찾는다.
- 변경 목록(diff)은 배열을 위치 기준으로 비교한다. 중간 원소를 지우면 뒤 원소가 모두 바뀐 것처럼 보인다 (결과는 정확).
- 소스 컨트롤 체크아웃, 커맨드렛, Dry-run 창은 아직 없다.
