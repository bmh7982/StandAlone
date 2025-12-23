# Keil MDK-ARM 프로젝트 설정 가이드

## 1단계: 새 프로젝트 생성

1. Keil μVision 실행
2. **Project → New μVision Project** 선택
3. 프로젝트 저장 위치: `cortex-write` 폴더
4. 프로젝트 이름: `CMSIS-DAP-Programmer`

## 2단계: Device 선택

1. Device 선택 창에서 **STMicroelectronics** 선택
2. **STM32F103C8** 선택
3. **OK** 클릭

## 3단계: Run-Time Environment 설정

CMSIS-DAP 프로젝트에 필요한 컴포넌트 선택:

### Device 섹션
- ✓ **Startup** - STM32F103C8용 스타트업 파일

### CMSIS 섹션
- ✓ **CORE** - Cortex-M 코어 지원

### Device 섹션 → STM32Cube Framework (HAL)
- ✓ **STM32CubeMX** - HAL 드라이버 자동 설정

또는 수동으로 필요한 HAL 드라이버 선택:
- ✓ **RCC** - 클럭 설정
- ✓ **GPIO** - GPIO 제어
- ✓ **UART** - USART3 통신
- ✓ **SPI** - SD 카드 통신 (나중 단계)

**OK** 클릭하여 파일 추가

## 4단계: 소스 파일 추가

Project 창에서 파일 그룹 생성 및 추가:

### Application 그룹
- `Src/main.c`
- `Src/system_init.c`

### Include Paths 설정
1. **Project → Options for Target**
2. **C/C++** 탭 선택
3. **Include Paths** 에 추가:
   - `.\Inc`
   - `.\Drivers\STM32F1xx_HAL_Driver\Inc`
   - `.\Drivers\CMSIS\Device\ST\STM32F1xx\Include`
   - `.\Drivers\CMSIS\Include`

## 5단계: 컴파일러 옵션 설정

**Project → Options for Target** (Alt+F7)

### Target 탭
- **Use MicroLIB**: ✓ 체크 (필수!)
- **Code Generation**:
  - ARM Compiler: Use default compiler version 6

### C/C++ 탭
- **Optimization**: Level 1 (-O1)
- **Define**: `USE_HAL_DRIVER,STM32F103xB`
- **Warnings**: All Warnings

### Linker 탭
- **Use Memory Layout from Target Dialog**: ✓ 체크
- **Scatter File**: 자동 생성됨

### Debug 탭
- **Debugger**: 사용하는 디버거 선택 (ST-Link, J-Link 등)

## 6단계: 메모리 설정 확인

**Options for Target → Target** 탭에서:

- **IROM1**:
  - Start: `0x08000000`
  - Size: `0x10000` (64KB)
- **IRAM1**:
  - Start: `0x20000000`
  - Size: `0x5000` (20KB)

## 7단계: 빌드 테스트

1. **Project → Build Target** (F7)
2. 빌드 메시지 확인:
   - 0 Error(s)
   - 경고는 검토
3. 메모리 사용량 확인:
   - Code (Flash): 64KB 이내
   - RW Data + ZI Data (RAM): 20KB 이내

## 8단계: 다운로드 및 테스트

1. Blue Pill 보드를 ST-Link에 연결
2. **Flash → Download** (F8)
3. 시리얼 터미널 (9600 baud) 연결
4. "READY\r\n" 메시지 확인
5. LED1 (PB12) 500ms 간격 깜빡임 확인

## 주의사항

### MicroLIB 사용 필수
- RAM 20KB 제약으로 인해 표준 C 라이브러리 대신 MicroLIB 사용
- `printf`, `scanf` 등 일부 함수 제한됨

### 최적화 레벨
- 개발 중: -O0 또는 -O1
- 릴리스: -O2 또는 -O3
- Flash 공간 부족 시 최적화 레벨 조정

### STM32 HAL 드라이버
- STM32CubeMX로 자동 생성 가능
- 또는 STM32Cube_FW_F1 패키지에서 수동 복사

## 다음 단계

이제 Step 1 완료! 다음 단계로 진행:
- **Step 2**: UART 통신 모듈 추가
- **Step 3**: SD 카드 및 FatFS
- **Step 4**: Intel HEX 파서
- 이후 단계들...

## 트러블슈팅

### 컴파일 에러: "stm32f1xx_hal.h not found"
→ HAL 드라이버 경로가 Include Paths에 추가되었는지 확인

### 링크 에러: "undefined reference to HAL_Init"
→ HAL 라이브러리 소스 파일들이 프로젝트에 추가되었는지 확인

### 빌드 성공했지만 보드가 작동하지 않음
→ startup 파일과 system_stm32f1xx.c 파일이 포함되었는지 확인
→ 클럭 설정이 올바른지 확인 (HSE 8MHz)
