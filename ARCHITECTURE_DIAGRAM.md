# Voltage Detection System - Architecture Diagram

## Overall System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     NUCLEO F401 BOARD                           │
│                                                                 │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │            STM32F401 MICROCONTROLLER                       │ │
│  │                                                            │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │                  ARM Cortex-M4 CPU                   │ │ │
│  │  │                  Running main.c                      │ │ │
│  │  └──────────┬───────────────────────────────────────────┘ │ │
│  │             │                                              │ │
│  │   ┌─────────┴──────────────────────────────────────────┐  │ │
│  │   │  Calls: VoltageDetector_GetRawValue()              │  │ │
│  │   │         VoltageDetector_GetVoltage()               │  │ │
│  │   │         VoltageDetector_UpdateSampleCount()        │  │ │
│  │   └─────────┬──────────────────────────────────────────┘  │ │
│  │             │                                              │ │
│  │   ┌─────────▼──────────────────────────────────────────┐  │ │
│  │   │     VOLTAGE DETECTOR MODULE                        │  │ │
│  │   │  (voltage_detector.h / voltage_detector.c)        │  │ │
│  │   │                                                    │  │ │
│  │   │  ├─ VoltageDetector_Init()                        │  │ │
│  │   │  ├─ VoltageDetector_Start()                       │  │ │
│  │   │  ├─ VoltageDetector_GetRawValue()                 │  │ │
│  │   │  ├─ VoltageDetector_GetVoltage()                  │  │ │
│  │   │  └─ VoltageDetector_UpdateSampleCount()           │  │ │
│  │   └─────────┬──────────────────────────────────────────┘  │ │
│  │             │                                              │ │
│  │   ┌─────────▼──────────────────────────────────────────┐  │ │
│  │   │  DIRECT REGISTER ACCESS (No HAL needed)           │  │ │
│  │   │                                                    │  │ │
│  │   │  RCC_AHB1ENR   (Clock control)                    │  │ │
│  │   │  RCC_APB2ENR   (Clock control)                    │  │ │
│  │   │  GPIOA_MODER   (PA0 mode config)                  │  │ │
│  │   │  ADC1_CR2      (ADC control)                      │  │ │
│  │   │  ADC1_SMPR2    (Sampling time)                    │  │ │
│  │   │  ADC1_SQR3     (Channel select)                   │  │ │
│  │   │  ADC1_DR       (Data/Status)                      │  │ │
│  │   └─────────┬──────────────────────────────────────────┘  │ │
│  │             │                                              │ │
│  │   ┌─────────▼──────────────────────────────────────────┐  │ │
│  │   │         HARDWARE PERIPHERALS                       │  │ │
│  │   │                                                    │  │ │
│  │   │  ┌──────────────────────────────────────────────┐ │  │ │
│  │   │  │  RCC (Reset & Clock Control)                │ │  │ │
│  │   │  │  Distributes clocks to GPIO and ADC        │ │  │ │
│  │   │  └──────────────────────────────────────────────┘ │  │ │
│  │   │                                                    │  │ │
│  │   │  ┌──────────────────────────────────────────────┐ │  │ │
│  │   │  │  GPIOA (GPIO Port A)                         │ │  │ │
│  │   │  │  PA0: Analog Input Pin                      │ │  │ │
│  │   │  └────────────┬─────────────────────────────────┘ │  │ │
│  │   │               │                                    │  │ │
│  │   │  ┌────────────▼──────────────────────────────────┐ │  │ │
│  │   │  │  ADC1 (Analog to Digital Converter)          │ │  │ │
│  │   │  │  • Samples voltage on PA0                   │ │  │ │
│  │   │  │  • Converts to 12-bit value (0-4095)       │ │  │ │
│  │   │  │  • Takes 84 clock cycles per sample        │ │  │ │
│  │   │  └────────────┬─────────────────────────────────┘ │  │ │
│  │   │               │                                    │  │ │
│  │   └───────────────┼────────────────────────────────────┘  │ │
│  │                   │                                        │ │
│  └───────────────────┼────────────────────────────────────────┘ │
│                      │                                           │
└──────────────────────┼───────────────────────────────────────────┘
                       │
                       │ PA0 Pin (Analog Input)
                       │
                       ▼
        ┌─────────────────────────────────┐
        │   VOLTAGE SOURCE                │
        │   (0V to 3.3V)                  │
        │                                 │
        │   Connected to:                 │
        │   • Nucleo PA0 pin             │
        │   • GND pin (common reference) │
        └─────────────────────────────────┘
```

---

## Software Initialization Sequence

```
main()
  │
  ├─► HAL_Init()
  │   └─ Initialize HAL library
  │
  ├─► SystemClock_Config()
  │   └─ Configure system clock (HSI 16 MHz)
  │
  ├─► VoltageDetector_Init()
  │   │
  │   ├─► RCC_AHB1ENR |= (1 << 0)
  │   │   └─ Enable GPIOA clock
  │   │
  │   ├─► RCC_APB2ENR |= (1 << 8)
  │   │   └─ Enable ADC1 clock
  │   │
  │   ├─► GPIOA_MODER |= (3 << 0)
  │   │   └─ Set PA0 to analog mode
  │   │
  │   ├─► ADC1_SMPR2 |= (6 << 0)
  │   │   └─ Set 84-cycle sampling time
  │   │
  │   ├─► ADC1_SQR3 |= (0 << 0)
  │   │   └─ Select channel 0 (PA0)
  │   │
  │   ├─► ADC1_CR2 |= (1 << 0)
  │   │   └─ Enable ADC hardware
  │   │
  │   └─► Initialize voltage_detector structure
  │       ├─ raw_value = 0
  │       ├─ voltage = 0.0f
  │       └─ sample_count = 0
  │
  ├─► VoltageDetector_Start()
  │   └─► ADC1_CR2 |= (1 << 30)
  │       └─ Start continuous conversions
  │
  └─► while(1) - Main Loop
      │
      ├─► VoltageDetector_GetRawValue()
      │   │
      │   ├─ ADC1_CR2 |= (1 << 30)
      │   │ └─ Trigger conversion
      │   │
      │   ├─ while (!(ADC1_DR & (1 << 31)))
      │   │ └─ Wait for EOC flag
      │   │
      │   ├─ raw = ADC1_DR & 0xFFF
      │   │ └─ Read 12-bit value
      │   │
      │   └─ voltage = raw × (3.3/4095)
      │     └─ Calculate voltage
      │
      ├─► VoltageDetector_GetVoltage()
      │   └─ return detector->voltage
      │
      ├─► VoltageDetector_UpdateSampleCount()
      │   └─ detector->sample_count++
      │
      ├─► HAL_Delay(100)
      │   └─ Wait 100ms (10 Hz sampling)
      │
      └─► REPEAT
```

---

## ADC Conversion Timeline

```
Time     Event                    Register State              Action
────     ─────────────────────    ──────────────────────      ──────────────
 0μs     Start conversion         ADC1_CR2[30] = 1            ADC begins sampling
         triggered
         
 1μs     Sampling starts          Capacitor charging         Reading analog signal
         on PA0                    from input voltage         
         
10μs     Sampling complete        Voltage captured           Conversion phase begins
         
20μs     Conversion complete      ADC1_DR has value          Data ready
         
25μs     Program reads            while(!(ADC1_DR           Loop condition checks
         ADC1_DR                   & (1<<31)))               EOC flag
         
25.5μs   EOC flag detected        ADC1_DR[31] = 1            Program detects ready
         
26μs     Exit conversion loop      Program continues          Read the result
         
26.5μs   Mask to 12 bits          raw & 0xFFF               Extract only data bits
         
27μs     Calculate voltage        raw × VOLTAGE_SCALE        Multiply by 0.000806
         
28μs     Store results            detector->voltage          Voltage available
                                  detector->raw_value
         
100ms    Next conversion          (100ms loop delay)         Back to start
```

---

## Register Map and Addresses

```
Memory Map (Simplified)
═════════════════════════════════════════════════════════════

0x00000000 ─────────────────────────────────────────── Code Flash
           │  STM32F401 Program & Constants           │
           │                                           │
0x08020000 ─────────────────────────────────────────── 

0x20000000 ─────────────────────────────────────────── RAM
           │  Variables, Stack                         │
           │                                           │
0x20018000 ─────────────────────────────────────────── 

0x40000000 ─────────────────────────────────────────── Peripherals
           │                                           │
0x40013FFF ─────────────────────────────────────────── 

           ┌─ 0x40020000 GPIOA Base
           │  └─ 0x00 MODER (PA0 = bits [1:0])
           │
           ├─ 0x40023800 RCC Base
           │  ├─ 0x30 AHB1ENR (GPIOA = bit [0])
           │  └─ 0x44 APB2ENR (ADC1 = bit [8])
           │
           └─ 0x40012000 ADC1 Base
              ├─ 0x08 CR2 (ADON=bit[0], SWSTART=bit[30])
              ├─ 0x10 SMPR2 (CH0 time=bits[2:0])
              ├─ 0x34 SQR3 (CH0 select=bits[4:0])
              └─ 0x4C DR (DATA=bits[11:0], EOC=bit[31])
```

---

## Data Flow: Voltage → Raw → Voltage

```
External World                    Microcontroller Hardware    Software

Voltage Source                    
   │
   │ (0V to 3.3V)               
   │
   ▼
PA0 Pin ─────────────────────►  ADC Sample Input
                                 │
                                 ├─ Capacitor samples voltage
                                 │
                                 ├─ Compares with reference (3.3V)
                                 │
                                 └─ Generates 12-bit digital value
                                    │
                                    ▼
                                  ADC1_DR Register
                                  Bits [11:0] = 0-4095
                                    │
                                    │ VoltageDetector_GetRawValue()
                                    │ reads ADC1_DR & 0xFFF
                                    │
                                    ▼
                                  detector->raw_value
                                    │
                                    │ Multiply by VOLTAGE_SCALE
                                    │ (3.3 / 4095)
                                    │
                                    ▼
                                  detector->voltage (0-3.3V)
                                    │
                                    │ VoltageDetector_GetVoltage()
                                    │ returns the value
                                    │
                                    ▼
                                  Application
                                  Has voltage in volts!
```

---

## File Organization

```
BreadDemo/
│
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   │   └─ #include "voltage_detector.h"
│   │   │
│   │   ├── voltage_detector.h
│   │   │   ├─ Register macros
│   │   │   ├─ VOLTAGE_SCALE definition
│   │   │   ├─ VoltageDetector_t struct
│   │   │   └─ Function prototypes
│   │   │
│   │   └── stm32f4xx_hal_conf.h
│   │       └─ HAL module enables
│   │
│   └── Src/
│       ├── main.c
│       │   ├─ VoltageDetector_t voltage_detector;
│       │   ├─ VoltageDetector_Init()
│       │   ├─ VoltageDetector_Start()
│       │   └─ while(1) reading loop
│       │
│       └── voltage_detector.c
│           ├─ Register definitions
│           ├─ VoltageDetector_Init()
│           ├─ VoltageDetector_Start()
│           ├─ VoltageDetector_GetRawValue()
│           ├─ VoltageDetector_GetVoltage()
│           └─ VoltageDetector_UpdateSampleCount()
│
├── Drivers/
│   ├── CMSIS/
│   │   └─ ARM Cortex definitions
│   │
│   └── STM32F4xx_HAL_Driver/
│       ├── Inc/
│       │   └─ HAL header files (GPIO, RCC, etc)
│       │
│       └── Src/
│           └─ HAL implementation (not used by our ADC code)
│
└── Documentation/
    ├── VOLTAGE_DETECTION_README.md
    ├── IMPLEMENTATION_FIX_DETAILS.md
    ├── COMPILATION_FIX_SUMMARY.md
    ├── REGISTER_OPERATIONS_EXPLAINED.md
    └── ARCHITECTURE_DIAGRAM.md (this file)
```

---

## Signal Path Diagram

```
┌─────────────────┐
│ External Voltage│
│   0V to 3.3V    │
└────────┬────────┘
         │
         │ Analog Signal
         ▼
    ┌────────┐
    │   PA0  │  (GPIO Pin A0)
    └────┬───┘
         │
         │ Sampled for 84 clocks
         ▼
    ┌────────────┐
    │ ADC Sample │
    │ Capacitor  │
    └────┬───────┘
         │
         │ Conversion Process
         ▼
    ┌────────────────┐
    │ ADC Converter  │
    │ Compares to    │
    │ 3.3V Reference │
    └────┬───────────┘
         │
         │ 12-bit digital result
         ▼
    ┌────────────────┐
    │  ADC1_DR[11:0] │
    │  0 to 4095     │
    └────┬───────────┘
         │
         │ Stored in register
         ▼
    ┌────────────────────┐
    │ Software reads via │
    │ register address   │
    │ 0x4001204C        │
    └────┬───────────────┘
         │
         │ Masked to 12 bits
         ▼
    ┌────────────────────┐
    │ raw_value variable │
    │ (0 to 4095)        │
    └────┬───────────────┘
         │
         │ Multiply by 0.000806
         │ (3.3 / 4095)
         ▼
    ┌────────────────────┐
    │ voltage variable   │
    │ (0.0 to 3.3V)      │
    └────┬───────────────┘
         │
         │ Stored in struct
         ▼
    ┌────────────────────┐
    │ Application has    │
    │ voltage reading    │
    │ Every 100ms        │
    └────────────────────┘
```

---

## State Machine: ADC Conversion

```
                    ┌─────────────┐
                    │   IDLE      │
                    │             │
                    └──────┬──────┘
                           │
                    [Conversion trigger]
                    ADC1_CR2 |= (1<<30)
                           │
                           ▼
                    ┌─────────────┐
                    │  SAMPLING   │◄──────────┐
                    │  84 clocks  │           │
                    │             │           │
                    └──────┬──────┘           │
                           │                 │
                    [Sampling done]          │
                           │                 │
                           ▼                 │
                    ┌─────────────┐          │
                    │ CONVERTING  │          │
                    │  Comparing  │          │
                    │  to VREF    │          │
                    └──────┬──────┘          │
                           │                 │
                    [Done converting]        │
                           │                 │
                           ▼                 │
                    ┌─────────────┐          │
                    │  EOC SET    │          │
                    │ Data Ready  │          │
                    │ in ADC1_DR  │          │
                    └──────┬──────┘          │
                           │                 │
                    [Software reads result]  │
                           │                 │
                           ▼                 │
                    ┌─────────────┐          │
                    │ CONTINUOUS  │──────────┘
                    │ MODE        │
                    │ Auto-restart│
                    └─────────────┘
```

---

## Timing Diagram

```
                100ms Loop Cycle
    ╔═══════════════════════════════════════╗
    ║                                       ║
    ║  ┌──────────────────────────────────┐ ║
    ║  │ VoltageDetector_GetRawValue()    │ ║
    ║  │ ~30μs execution time             │ ║
    ║  └──────────────────────────────────┘ ║
    ║    │                                  ║
    ║    ├─ Trigger ADC (1μs)               ║
    ║    ├─ Wait for completion (20μs)      ║
    ║    ├─ Read and calculate (5μs)        ║
    ║    └─ Return (4μs)                    ║
    ║                                       ║
    ║  ┌──────────────────────────────────┐ ║
    ║  │ VoltageDetector_GetVoltage()     │ ║
    ║  │ ~1μs execution time              │ ║
    ║  └──────────────────────────────────┘ ║
    ║    └─ Return value                    ║
    ║                                       ║
    ║  ┌──────────────────────────────────┐ ║
    ║  │ VoltageDetector_UpdateSampleCount│ ║
    ║  │ ~1μs execution time              │ ║
    ║  └──────────────────────────────────┘ ║
    ║    └─ Increment counter              ║
    ║                                       ║
    ║  ┌──────────────────────────────────┐ ║
    ║  │ HAL_Delay(100)                   │ ║
    ║  │ ~100,000μs = 100ms               │ ║
    ║  └──────────────────────────────────┘ ║
    ║    └─ Wait 100ms                     ║
    ║                                       ║
    ║  Total: ~100ms → ~10 samples/second  ║
    ║                                       ║
    ╚═══════════════════════════════════════╝
    (Loop repeats every 100ms)
```

---

## Summary

- **Hardware**: STM32F401 microcontroller with ADC1 and GPIOA
- **Pin**: PA0 receives 0-3.3V analog input
- **Software**: Direct register access (no HAL ADC driver needed)
- **Precision**: 12-bit ADC = 4096 levels (3.3V / 4096)
- **Speed**: 10 samples per second (100ms interval)
- **Storage**: Structure tracks raw value, voltage, and sample count
- **Flow**: Voltage → PA0 → ADC hardware → ADC1_DR register → Software calculation → Application

