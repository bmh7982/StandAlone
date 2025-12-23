# CMSIS-DAP Programmer for STM32F103C8T6

STM32F103C8T6 Blue Pill 기반 ARM Cortex-M0/M3/M4 타겟용 CMSIS-DAP 프로그래머

## 프로젝트 개요

이 프로젝트는 STM32F103C8T6 Blue Pill 보드를 사용하여 ARM Cortex-M 시리즈 MCU를 프로그래밍하는 독립형(Stand-alone) CMSIS-DAP 프로그래머입니다. SD 카드에 저장된 Intel HEX 파일을 읽어 SWD 프로토콜을 통해 타겟 MCU에 프로그래밍합니다.

### 주요 기능

- **자동 타겟 감지**: Cortex-M0/M3/M4 자동 인식 (IDCODE 기반)
- **SD 카드 지원**: FatFS를 통한 Intel HEX 파일 읽기
- **UART 제어**: PC로부터 명령 수신 및 결과 전송
- **프로그래밍 검증**: 자동 베리파이 기능
- **상태 표시**: LED를 통한 진행 상황 및 결과 표시

## 하드웨어 사양

### MCU
- **보드**: STM32F103C8T6 Blue Pill
- **CPU**: ARM Cortex-M3, 72MHz
- **메모리**:
  - Flash: 64KB
  - RAM: 20KB

### 핀 할당

#### UART 통신 (USART3)
| 기능 | 핀 | 설정 |
|------|-----|------|
| TX | PB10 | 송신 |
| RX | PB11 | 수신 |
| Baudrate | - | 9600 bps |

#### SWD 타겟 연결
| 기능 | 핀 | 설명 |
|------|-----|------|
| SWDIO | PA2 | 데이터 입출력 |
| SWCLK | PA4 | 클럭 신호 |
| RESET | PA6 | 타겟 리셋 |

#### 상태 LED
| LED | 핀 | 기능 |
|-----|-----|------|
| LED1 | PB12 | 진행 상황 (깜빡임) |
| LED2 | PB13 | 결과 표시 (성공/실패) |

#### SD 카드 (SPI)
- SPI1 또는 SPI2 사용
- 핀은 STM32 HAL에서 자동 할당

## 프로젝트 구조

```
cortex-write/
├── Src/
│   ├── main.c              # 메인 루프 및 통합
│   ├── system_init.c       # 시스템 초기화
│   ├── uart.c              # UART 통신
│   ├── sd_card.c           # SD 카드 및 FatFS
│   ├── hex_parser.c        # Intel HEX 파서
│   ├── led_control.c       # LED 제어
│   └── swd_dap.c           # SWD 프로토콜 및 플래시 프로그래밍
├── Inc/
│   ├── main.h
│   ├── config.h            # 모든 설정 매크로
│   ├── uart.h
│   ├── sd_card.h
│   ├── hex_parser.h
│   ├── led_control.h
│   └── swd_dap.h
├── Drivers/                # STM32 HAL 드라이버
└── MDK-ARM/
    ├── cmsys-load.uvprojx  # Keil 프로젝트 파일
    └── startup_stm32f103xb.s   # 스타트업 파일
```

### 현재 구현 상태 (Step 1 완료)

**완료된 기능:**
- ✅ 시스템 클록 설정 (72MHz)
- ✅ GPIO 초기화 (LED, SWD 핀)
- ✅ USART3 초기화 (9600 baud)
- ✅ LED 초기화
- ✅ "READY\r\n" 메시지 전송

**구현 예정:**
- ⏳ UART 명령 수신 및 파싱 (Step 2)
- ⏳ SD 카드 및 FatFS (Step 3)
- ⏳ Intel HEX 파서 (Step 4)
- ⏳ LED 제어 패턴 (Step 5)
- ⏳ SWD 프로토콜 (Step 6)
- ⏳ 플래시 프로그래밍 (Step 7)
- ⏳ 최종 통합 (Step 8)

## 빌드 방법

### 개발 환경
- **IDE**: Keil MDK-ARM (μVision)
- **컴파일러**: ARM Compiler 6
- **라이브러리**: STM32 HAL, FatFS, CMSIS-DAP

### 빌드 설정
1. Keil μVision에서 프로젝트 열기
2. Target Options 확인:
   - Device: STM32F103C8
   - Use MicroLIB: Yes (체크)
3. Project → Build Target (F7)

### 메모리 사용량 확인
- RAM 사용량이 20KB 이내인지 확인
- Flash 사용량이 64KB 이내인지 확인

## 사용 방법

### 1. 하드웨어 연결

1. **Blue Pill 전원 연결**
2. **SD 카드 모듈 연결** (SPI 핀에 연결)
3. **UART-USB 변환기 연결** (PB10, PB11)
4. **타겟 ARM 보드 SWD 연결** (PA2, PA4, PA6)
5. **LED 연결 확인** (PB12, PB13)

### 2. SD 카드 준비

1. SD 카드를 FAT32로 포맷
2. Intel HEX 파일을 SD 카드 루트에 복사
3. SD 카드를 모듈에 삽입

### 3. 프로그래밍 실행

1. 시리얼 터미널 연결 (9600 baud)
2. 보드 리셋 후 `READY\r\n` 메시지 확인
3. 명령 전송:
   ```
   FILE: /firmware.hex\r\n
   ```
4. LED로 진행 상황 확인
5. 결과 수신:
   - 성공: `OK\r\n` + LED2 켜짐
   - 실패: 에러 코드 + LED2 빠른 깜빡임

## 통신 프로토콜

### 명령 형식
```
FILE: <파일경로>\r\n
```

예시:
```
FILE: /test.hex\r\n
FILE: /firmware/app.hex\r\n
```

### 응답 코드

| 코드 | 의미 |
|------|------|
| `OK\r\n` | 프로그래밍 성공 |
| `NG\r\n` | 일반 실패 |
| `ERR_SD_MOUNT\r\n` | SD 카드 마운트 실패 |
| `ERR_FILE_NOT_FOUND\r\n` | HEX 파일을 찾을 수 없음 |
| `ERR_HEX_PARSE\r\n` | HEX 파일 파싱 오류 |
| `ERR_TARGET_CONNECT\r\n` | 타겟 연결 실패 |
| `ERR_PROGRAM_FAIL\r\n` | 플래시 프로그래밍 실패 |
| `ERR_VERIFY_FAIL\r\n` | 베리파이 실패 |

## 동작 흐름

```
1. 시스템 초기화
   ↓
2. SD 카드 마운트
   ↓
3. UART로 "READY" 전송
   ↓
4. 명령 대기
   ↓
5. "FILE:" 명령 수신
   ↓
6. SD 카드에서 HEX 파일 열기
   ↓
7. SWD로 타겟 연결
   ↓
8. IDCODE 읽어서 MCU 타입 감지
   ↓
9. 플래시 unlock 및 erase
   ↓
10. HEX 파일 파싱하며 프로그래밍
    ↓
11. 파일 처음으로 돌아가 베리파이
    ↓
12. 플래시 lock 및 타겟 리셋
    ↓
13. 결과 전송 ("OK" 또는 에러 코드)
    ↓
14. 명령 대기로 복귀
```

## LED 상태 표시

| 상태 | LED1 (PB12) | LED2 (PB13) |
|------|-------------|-------------|
| 대기 | 꺼짐 | 꺼짐 |
| 프로그래밍 중 | 100ms 깜빡임 | 꺼짐 |
| 성공 | 꺼짐 | 켜짐 |
| 에러 | 꺼짐 | 200ms 빠른 깜빡임 |

## 지원 타겟 MCU

### Cortex-M0
- IDCODE: `0x0BB11477`
- 예: STM32F0 시리즈

### Cortex-M3
- IDCODE: `0x4BA00477`
- 예: STM32F1 시리즈

### Cortex-M4
- IDCODE: `0x4BA01477`
- 예: STM32F4 시리즈

## 트러블슈팅

### SD 카드 마운트 실패
- SD 카드가 FAT32로 포맷되어 있는지 확인
- SD 카드 모듈 SPI 연결 확인
- 전원 공급 확인 (3.3V)

### 타겟 연결 실패
- SWD 핀 연결 확인 (SWDIO, SWCLK, RESET)
- 타겟 보드 전원 확인
- 타겟 보드의 SWD 핀이 올바른지 확인

### 프로그래밍 실패
- HEX 파일이 타겟 MCU용인지 확인
- 타겟 플래시 메모리 크기 확인
- 타겟 MCU가 보호되어 있지 않은지 확인

### 베리파이 실패
- 타겟 MCU의 플래시 메모리 상태 확인
- 전원 안정성 확인
- SWD 연결 안정성 확인

## 개발 정보

### 8단계 개발 프로세스

이 프로젝트는 모듈별로 단계적으로 개발되었습니다:

1. **Step 1**: 프로젝트 기본 구조 및 초기화
2. **Step 2**: UART 통신 모듈
3. **Step 3**: SD 카드 및 FatFS
4. **Step 4**: Intel HEX 파서
5. **Step 5**: LED 제어 모듈
6. **Step 6**: CMSIS-DAP/SWD 프로토콜
7. **Step 7**: 플래시 프로그래밍
8. **Step 8**: 최종 통합 및 완성

### 라이선스

이 프로젝트는 MIT 라이선스를 따릅니다.

## 참고 자료

- [CMSIS-DAP Specification](https://arm-software.github.io/CMSIS_5/DAP/html/index.html)
- [ARM Debug Interface Architecture Specification (ADIv5)](https://developer.arm.com/documentation/ihi0031/latest/)
- [Intel HEX Format](https://en.wikipedia.org/wiki/Intel_HEX)
- [STM32F103C8T6 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
