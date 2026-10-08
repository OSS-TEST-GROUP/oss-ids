# Release Notes

## v0.2.0

상태 전이 위반 탐지기(`StateTransitionViolationDetector`) 추가, 신호 불일치 탐지기의 과도기 오탐 방지 메커니즘 도입, 정책 규칙 모델 확장 등이 개선되었습니다.

### 주요 기능

- **상태 전이 위반 탐지기(`StateTransitionViolationDetector`) 추가**
  - 차량 도어 상태, 기어 모드, 제어 모드 등 시계열 이벤트에 대해 허용되지 않거나 비정상적인 상태 전이를 탐지하는 기능(`STATE_TRANSITION_RULE`) 제공
  - 토픽별 상태 머신 추적 및 연계 신호 조건(속도 등)에 따른 전이 위반 판정 지원
  - 정책 레지스트리(`PolicyRegistry`) 및 단계형 분석 파이프라인(`AnalysisDetectionStage`) 연계
- **신호 간 불일치 탐지기(`SignalMismatchDetector`) 과도기 오탐 방지 및 안정화**
  - 상태 전환(토글) 시 신호 전파 지연으로 인한 일시적인 불일치를 무시하는 안정화 시간(`SETTLING_TIME_MS`, 기본 500ms 유예) 메커니즘 지원
  - `ObservationStore`에 토픽별 상태 변경 시점(`lastStateChangeTimestampMs`) 추적 로직 추가
  - 비교 조건 불일치 시 규칙 평가 건너뛰기 처리 및 최소 연속 탐지 횟수(`MIN_CONSECUTIVE`) 기반 경합 방지 로직 개선
- **정책 설정 모델(`PolicyConfig`) 확장 및 버전 관리**
  - 정책 설정 파일 버전(`VERSION 1.1`) 지원
  - 상태 전이 규칙(`StateTransitionViolationRule`) 및 안정화 시간(`settlingTimeMs`) 설정 구조 확장


## v0.1.0

DDS 환경에서 데이터를 수집하고 보안 정책을 적용해 탐지 결과를 전달하는 OSS-IDS의 첫 번째 릴리즈입니다.

### 포함된 Lib v0.1.0 주요 기능

- 수집 데이터의 분석 흐름을 관리하는 단계형 탐지 파이프라인 제공
- 신호 간 상태 불일치, 메시지 주기 범위 이탈, RTPS Vendor ID 허용 목록 위반 탐지 지원
- 정책 규칙과 심각도, 탐지 결과 및 증거 데이터를 표현하는 공통 모델 제공
- 소스 및 토픽별 최신 관측 데이터를 조회할 수 있는 저장소 제공
- 로깅, 명령행 프로그램 수명 주기, 데이터 변환 등 공통 유틸리티 제공
- C++17, Conan 2, CMake 기반의 `common` 및 `analysis` 정적 라이브러리 패키징 지원
- 분석 단계와 탐지기 동작을 검증하는 GoogleTest 기반 단위 테스트 제공

### OSS-IDS 주요 기능

- `SecurityClient`의 수집, 분석, 경고 단계를 분리한 IDS 처리 파이프라인 제공
- Fast DDS를 이용한 `DoorSwitch` 및 `DoorUnlockLockIndicator` 토픽 수집 지원
- 런타임 설정과 탐지 정책을 분리한 JSON 기반 구성 및 명령행 옵션 지원
- 탐지 결과를 `DetectionReport` DDS 토픽으로 발행하는 경고 전송 기능 제공
- `SecurityManager`를 통한 DDS 도메인 간 탐지 보고서 브리지 및 레거시 토픽 연계 지원
- 차량 상태와 탐지 보고서용 IDL 및 Fast DDS 타입 생성 구성 제공
- SecurityClient의 설정, 수집, 분석, 경고 및 애플리케이션 수명 주기를 검증하는 테스트 제공
- `security_processor`, `attack_env`, `security_monitor`로 구성된 Docker Compose 테스트 환경과 amd64/arm64 이미지 실행 지원
