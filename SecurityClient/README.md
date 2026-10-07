# SecurityClient

이 디렉터리는 기존의 `oss/security_client` 와 분리된, 단계형(staged) IDS 재구성 경로를 담고 있습니다.

## 1. 이 디렉터리의 목적

- 기존 구현을 건드리지 않고, `collection / analysis / alert` 3단계 구조를 별도 경로에서 검증한다.
- DDS 수집, 탐지 로직, 경고 발행을 독립적인 stage로 분리한다.
- JSON 기반 runtime 설정과 policy 설정을 실제로 읽고, 테스트로 검증한다.
- 향후 `ClientState` 중심의 단일 구현을 대체할 수 있는 기반을 마련한다.

## 2. 현재 상태

이 디렉터리는 단순한 스캐폴드가 아니라, 다음 수준까지 구현되어 있습니다.

- 상위 실행 경로: `SecurityClientApp`, `StageRuntime`
- 설정 계층: `SecurityClientConfigLoader`
- 수집 계층: `CollectionStage`, `IDataCollector`, `DdsCollector`, `ObservationStore`
- 분석 계층: `AnalysisDetectionStage`, `SignalMismatchDetector`, `CycleRuleDetector`, `RtpsHeaderRuleDetector`
- 경고 계층: `AlertResponseStage`, `DdsDetectionReportSink`, `NoopResponseHandler`
- 테스트 계층: `gtest` 기반 regression suite

즉, 현재는 “구조만 있는 scaffold”가 아니라, 실제 단계형 구현과 회귀 테스트가 함께 들어 있는 실험용 구현 경로입니다.

## 3. 디렉터리별 역할

### app/
- `SecurityClientApp`: 구성 로드, stage 생성, 초기화, 실행, 종료를 담당하는 composition root입니다.
- `StageRuntime`: staged path를 얇은 facade로 감싸서 실행할 수 있게 합니다.

### config/
- `SecurityClientConfigLoader`: runtime JSON과 policy JSON을 읽어 내부 설정 객체로 변환합니다.
- `sample_runtime_config.json`, `sample_policy_config.json`: 테스트와 데모용 샘플 설정입니다.

### collection/
- `CollectionStage`: 수집기를 관리하고 `ObservationStore`에 샘플을 반영합니다.
- `DdsCollector`: DDS topic을 수집하는 adapter입니다.
- `ObservationStore`: source/topic 기준 최신 샘플 캐시를 관리합니다.

### analysis/
- `AnalysisDetectionStage`: 수집된 샘플을 detector에 전달하고 `DetectionFinding`을 생성합니다.
- detectors/: `SignalMismatchDetector`, `CycleRuleDetector`, `RtpsHeaderRuleDetector`
- `PolicyRegistry`: detector가 정책 규칙을 가져다 쓸 수 있도록 연결합니다.

### alert/
- `AlertResponseStage`: 탐지 결과를 sink와 handler로 전달합니다.
- `DdsDetectionReportSink`: DetectionReport DDS 발행을 담당합니다.
- `NoopResponseHandler`: 현재는 안전한 no-op 대응 경로를 제공합니다.

### gtest/
- 각 stage별 회귀 테스트와 운영성 테스트를 분리해 두었습니다.
- `config`, `collection`, `analysis`, `alert`, `app` 테스트가 일부 구현되어 있고, `integration`은 향후 확장 영역입니다.

## 4. 기존 실행 방식과 staged 실행 방식 비교

이 경로는 기존 `oss/security_client` 구현과 분리되어 있지만, 이전 명령 스타일과 설정 의미를 함께 이해할 수 있도록 아래처럼 정리해 두는 것이 좋습니다.

### 4.1 기존(legacy) 실행 예시

기존 구현은 다음처럼 실행합니다.

```sh
cd build/Debug
./bin/security_client -i 0 -d 0 -c policy_rule/config_dds_v1.0.json
```

- `-i 0`: 클라이언트 식별자(legacy CLI에서 사용하는 문자열형 값)
- `-d 0`: DetectionReport 발행용 DDS 도메인 ID
- `-c policy_rule/config_dds_v1.0.json`: legacy 정책 파일 경로

이 명령은 `oss/security_client` 쪽의 기존 정책 파서와 DDS 모니터링 경로를 그대로 사용합니다.

### 4.2 staged 경로에서의 비교 실행 예시

staged 경로의 현재 실행 파일은 `SecurityClient`입니다. 가장 간단한 실행은 다음과 같습니다.

```sh
cd build/Debug
./bin/SecurityClient -i security-client-v2
```

이 명령은 기본값으로 `oss/SecurityClient/config/sample_runtime_config.json` 과 `oss/SecurityClient/config/sample_policy_config.json` 을 읽습니다.

기존 legacy 실행과 가장 비슷하게 policy 파일을 넘기려면 다음처럼 실행합니다.

```sh
cd build/Debug
./bin/SecurityClient -i security-client-v2 -c policy_rule/config_dds_v1.0.json
```

이제 `-c` 옵션은 staged 경로에서도 policy 파일 경로로 해석됩니다. 즉 위 명령은 다음 조합으로 동작합니다.

- runtime 설정: `oss/SecurityClient/config/sample_runtime_config.json`
- policy 설정: `policy_rule/config_dds_v1.0.json`

runtime 설정 파일까지 바꾸고 싶다면 `-r` 또는 `--runtime-config` 옵션을 함께 사용합니다.

```sh
cd build/Debug
./bin/SecurityClient -i security-client-v2 -d 0 -c policy_rule/config_dds_v1.0.json -r oss/SecurityClient/config/sample_runtime_config.json
```

현재 구현은 legacy와 동일한 내부 구조는 아니지만, 적어도 `-c` 의미와 DetectionReport publisher 유지 동작은 legacy 쪽에 더 가깝게 맞추는 방향으로 수정 중입니다.

### 4.3 설정 파일 의미 정리

| 목적 | 기존 구현 | staged 구현 |
| --- | --- | --- |
| 런타임/시스템 설정 | legacy 정책 파일 내부 의미에 함께 포함 | 기본값은 `oss/SecurityClient/config/sample_runtime_config.json`, 필요 시 `-r`로 override |
| 정책 규칙 데이터 | `-c policy_rule/config_dds_v1.0.json` | `-c policy_rule/config_dds_v1.0.json` 또는 staged policy 파일 |
| DetectionReport 발행 도메인 | `-d` 옵션으로 지정 | `-d` 옵션으로 sink domain override |

즉, legacy 실행은 “정책 파일 하나로 전체 동작을 설명”하는 구조이고, staged 실행은 “runtime 설정 + policy 설정을 분리한 구성”이다. 다만 최신 staged 구현에서는 legacy 사용성을 맞추기 위해 `-c` 와 `-d` 의 의미를 legacy 쪽에 가깝게 다시 맞추고 있다.

### 4.4 security_manager 연동 확인 예시

현재 수정된 staged 경로는 아래 순서로 `security_manager` 와 DetectionReport publisher match를 확인할 수 있다.

```sh
cd build/Debug
./bin/SecurityClient -i security-client-v2 -c policy_rule/config_dds_v1.0.json
```

별도 터미널에서:

```sh
cd build/Debug
./bin/security_manager -s 0 -d 0
```

확인 포인트:

- `SecurityClient` 는 즉시 종료하지 않고 collector / sink를 유지한다.
- `security_manager` 는 `matched publisher: topic=DetectionReport` 로그를 출력해야 한다.

이는 “publisher가 떠 있고 security_manager가 이를 DDS에서 인식한다”는 수준까지는 연동이 복구되었음을 뜻한다.

## 5. 빌드와 실행

이 경로는 아직 `oss/CMakeLists.txt`에 기본 연결되지 않은 별도 구현 경로입니다. 따라서 직접 타깃을 빌드해서 검증합니다.

### 빌드

```sh
cd build/Debug
cmake --build . --target SecurityClient SecurityClient_test
```

### 테스트 목록 확인

```sh
cd build/Debug
./bin/SecurityClient_test --gtest_list_tests
```

### 핵심 테스트 실행

```sh
cd build/Debug
./bin/SecurityClient_test --gtest_filter='SecurityClientConfigLoaderTest.*:CollectionStageTest.*:AlertResponseStageTest.*'
```

### CTest 실행

```sh
cd build/Debug
ctest --output-on-failure -R SecurityClientTest --verbose
```

## 6. 현재 구현된 테스트 포인트

현재 staged 경로에서 실제로 검증되는 핵심 영역은 다음과 같습니다.

- 설정 로더의 정상/오류 경로
- collection stage의 sample 저장 및 failure path
- detector 생성과 rule 기반 탐지
- alert sink / handler publish 경로
- app/runtime 실행 경로

이는 구조 분리 후 회귀를 막고, 기능이 깨지는지 빠르게 확인하기 위한 최소한의 테스트 집합입니다.

## 7. 개발 시 주의점

- 기존의 `oss/security_client` 구현은 그대로 두고, 이 경로는 별도로 검증합니다.
- 새로운 detector나 sink를 추가할 때는 반드시 `gtest`로 실패/정상 경로를 함께 검증합니다.
- `SecurityClientApp`의 lifecycle 순서(`configure -> initialize -> run -> stop -> finalize`)를 깨지 않도록 유의합니다.
- 운영성 강화는 단순히 “동작함”만 확인하는 것이 아니라, 잘못된 config나 실패 시나리오도 검증해야 합니다.

## 8. 향후 확장 방향

- `gtest/integration`에 실제 pipeline 연결 테스트 추가
- `FakeDetector`, `SpyStageLifecycle` 같은 테스트 더블 확장
- invalid config, stale cache, missing subscriber, publish failure 같은 운영 시나리오 강화
- 필요 시 `DdsDetectionReportSink`와 실제 DDS 통합 테스트 분리

이 문서는 staged rewrite의 현재 구현 위치와 테스트 기준을 한눈에 이해하기 위한 안내서입니다.
