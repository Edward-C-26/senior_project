# STM32F401 Voltage Detection System - Complete Implementation Guide

## Quick Start

1. **Hardware Setup**: Connect voltage source (0-3.3V) to **PA0** pin
2. **Compile**: Build the project (no missing file errors now)
3. **Deploy**: Program to Nucleo F401 board
4. **Monitor**: Read `voltage_detector.voltage` for voltage values

---

## What Was Implemented

A complete voltage detection system for the STM32F401 Nucleo board that:
- ✅ Reads analog voltage from PA0
- ✅ Converts to 12-bit digital value (0-4095)
- ✅ Calculates actual voltage (0-3.3V)
- ✅ Samples at 10 Hz (every 100ms)
- ✅ Tracks total sample count
- ✅ **Works without HAL ADC driver** (uses direct register access)

---

## The Problem & Solution

### Original Issue ❌
```
fatal error: stm32f4xx_hal_adc.h: No such file or directory
```

Initial implementation tried to use the HAL ADC driver, but the file wasn't in your project.

### Our Solution ✅
Switched to **direct register-based ADC implementation**. This approach:
- Doesn't require HAL ADC driver files
- Uses direct memory addresses to control hardware
- Results in smaller, faster code
- Teaches you how microcontrollers actually work

---

## Files Created/Modified

### New Files
1. **Core/Inc/voltage_detector.h** (49 lines)
   - Defines voltage detector interface
   - Register macros for direct access
   - Voltage calculation constants

2. **Core/Src/voltage_detector.c** (85 lines)
   - Direct register implementations
   - ADC initialization, reading, and calculation

3. **Documentation** (5 detailed guides):
   - VOLTAGE_DETECTION_README.md
   - IMPLEMENTATION_FIX_DETAILS.md
   - COMPILATION_FIX_SUMMARY.md
   - REGISTER_OPERATIONS_EXPLAINED.md
   - ARCHITECTURE_DIAGRAM.md

### Modified Files
1. **Core/Inc/stm32f4xx_hal_conf.h**
   - Disabled: `#define HAL_ADC_MODULE_ENABLED`

2. **Core/Inc/main.h**
   - Added: `#include "voltage_detector.h"`

3. **Core/Src/main.c**
   - Added: Global `voltage_detector` instance
   - Added: Initialization in main()
   - Added: Reading loop with 100ms delay

---

## How It Works

### Hardware Connection
```
Analog Voltage (0-3.3V)
         ↓
        PA0 (GPIOA Pin 0)
         ↓
    ADC1 Channel 0
         ↓
   12-bit converter
         ↓
    0 to 4095 value
```

### Software Flow
```
VoltageDetector_Init()
    ├─ Enable GPIO and ADC clocks
    ├─ Configure PA0 as analog input
    ├─ Set 84-cycle sampling time
    └─ Enable ADC hardware

main loop (every 100ms):
    ├─ VoltageDetector_GetRawValue()
    │  ├─ Start ADC conversion
    │  ├─ Wait for completion
    │  ├─ Read 12-bit value (0-4095)
    │  └─ Calculate voltage: raw × 3.3/4095
    │
    ├─ VoltageDetector_GetVoltage()
    │  └─ Return voltage in volts
    │
    └─ VoltageDetector_UpdateSampleCount()
       └─ Increment counter
```

### Voltage Calculation
```
Raw ADC value: 0 ────────────────── 4095
Voltage:       0V ────────────────── 3.3V

Formula: voltage = raw_value × (3.3 / 4095)

Examples:
  • raw = 0    → voltage = 0.00V
  • raw = 1024 → voltage = 0.83V
  • raw = 2048 → voltage = 1.65V
  • raw = 3072 → voltage = 2.47V
  • raw = 4095 → voltage = 3.30V
```

---

## Register-Based Implementation (Key Concept)

Instead of using HAL library functions, we directly access hardware registers:

```c
// Enable GPIO clock
RCC_AHB1ENR |= (1 << 0);

// Enable ADC clock
RCC_APB2ENR |= (1 << 8);

// Set PA0 to analog mode
GPIOA_MODER |= (3 << 0);

// Set 84-cycle sampling time
ADC1_SMPR2 |= (6 << 0);

// Start conversion
ADC1_CR2 |= (1 << 30);

// Wait for completion
while (!(ADC1_DR & (1 << 31)));

// Read 12-bit result
uint32_t raw = ADC1_DR & 0xFFF;
```

**Why this works:**
- Registers are hardware-accessible memory addresses
- No dependency on external driver files
- Direct control of microcontroller features
- Smaller code footprint
- Educational value

---

## Register Addresses (STM32F401)

| Register | Address | Purpose |
|----------|---------|---------|
| RCC_AHB1ENR | 0x40023830 | GPIO clock enable |
| RCC_APB2ENR | 0x40023844 | ADC clock enable |
| GPIOA_MODER | 0x40020000 | PA0 mode config (analog) |
| ADC1_CR2 | 0x40012008 | ADC control (enable, start) |
| ADC1_SMPR2 | 0x40012010 | Sampling time (84 cycles) |
| ADC1_SQR3 | 0x40012034 | Channel select (channel 0) |
| ADC1_DR | 0x4001204C | Data and status (12-bit result, EOC flag) |

---

## Data Structure

```c
typedef struct {
    uint32_t raw_value;        // ADC reading (0-4095)
    float voltage;             // Calculated voltage (0-3.3V)
    uint32_t sample_count;     // Total samples taken
} VoltageDetector_t;
```

**Access in your code:**
```c
// Get the latest reading
float v = voltage_detector.voltage;      // In volts
uint32_t raw = voltage_detector.raw_value;  // 0-4095
uint32_t count = voltage_detector.sample_count;  // Total samples
```

---

## API Reference

### VoltageDetector_Init(detector)
Initializes the ADC and GPIO hardware.
```c
VoltageDetector_Init(&voltage_detector);
```

### VoltageDetector_Start(detector)
Begins continuous ADC conversions.
```c
VoltageDetector_Start(&voltage_detector);
```

### VoltageDetector_GetRawValue(detector)
Triggers a conversion and returns the 12-bit ADC value (0-4095).
```c
uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
```

### VoltageDetector_GetVoltage(detector)
Returns the voltage in volts (0-3.3V).
```c
float voltage = VoltageDetector_GetVoltage(&voltage_detector);
```

### VoltageDetector_UpdateSampleCount(detector)
Increments the sample counter.
```c
VoltageDetector_UpdateSampleCount(&voltage_detector);
```

---

## Main Loop Example

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    // Initialize and start voltage detector
    VoltageDetector_Init(&voltage_detector);
    VoltageDetector_Start(&voltage_detector);
    
    while (1)
    {
        // Get voltage reading
        uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
        float voltage = VoltageDetector_GetVoltage(&voltage_detector);
        
        // Track samples
        VoltageDetector_UpdateSampleCount(&voltage_detector);
        
        // Your code here: use voltage value
        // Example: if (voltage > 2.5f) { /* do something */ }
        
        // Sample every 100ms (10 Hz)
        HAL_Delay(100);
    }
}
```

---

## Testing & Verification

### Step 1: Hardware Setup
1. Connect a voltage source to PA0
2. Connect GND to a GND pin on the board
3. Voltage should be between 0V and 3.3V

### Step 2: Compile & Deploy
1. Build the project (should compile without errors now)
2. Program the Nucleo F401 board

### Step 3: Monitor Values
```c
// Add to your code to see values
float v = voltage_detector.voltage;

// With debugger: Watch these variables
// voltage_detector.raw_value     (0-4095)
// voltage_detector.voltage       (0-3.3V)
// voltage_detector.sample_count  (increments every 100ms)
```

### Test Data
| Applied Voltage | Expected Raw | Expected Result |
|-----------------|--------------|-----------------|
| 0.0V | 0 | 0.000V |
| 1.0V | 1240 | 0.999V |
| 1.65V | 2048 | 1.649V |
| 2.0V | 2479 | 1.997V |
| 3.3V | 4095 | 3.300V |

---

## Common Issues & Solutions

### Issue: No voltage readings
**Solution:**
- Verify PA0 is connected to voltage source
- Check GND connection
- Confirm voltage is between 0-3.3V
- Try known voltage (1.65V or 3.3V)

### Issue: Voltage reading stuck at 0
**Solution:**
- Make sure VoltageDetector_Start() was called
- Check that VoltageDetector_GetRawValue() is called in loop
- Verify voltage source has current flowing

### Issue: Readings fluctuating wildly
**Solution:**
- Add external capacitor (0.1μF) on PA0
- Implement averaging filter
- Increase ADC sampling time if possible
- Check for noise in voltage source

### Issue: Compilation errors
**Solution:**
- Ensure `#include "voltage_detector.h"` is in main.h
- Check that voltage_detector.c is in project
- Verify HAL_ADC_MODULE is commented out in hal_conf.h

---

## Enhancement Ideas

### 1. Add UART Logging
```c
// Print voltage to terminal
printf("Voltage: %.2f V\r\n", voltage_detector.voltage);
printf("Samples: %ld\r\n", voltage_detector.sample_count);
```

### 2. Implement Averaging Filter
```c
#define FILTER_SIZE 10
static float voltage_samples[FILTER_SIZE];

float GetAveragedVoltage() {
    float sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += voltage_samples[i];
    }
    return sum / FILTER_SIZE;
}
```

### 3. Add Threshold Detection
```c
if (voltage_detector.voltage > 2.5f) {
    // Voltage exceeded threshold - take action
}
```

### 4. Implement Voltage Counting
```c
uint32_t voltage_count = 0;
if (voltage_detector.voltage > 1.65f) {
    voltage_count++;
}
```

### 5. Add Multiple Channels
- Modify ADC1_SQR3 to select different channels
- Create multiple VoltageDetector_t instances
- Read from multiple analog pins

---

## Documentation Map

| Document | Purpose | Read When |
|----------|---------|-----------|
| **VOLTAGE_DETECTION_README.md** | System overview | Getting started |
| **IMPLEMENTATION_FIX_DETAILS.md** | Problem analysis & solution | Understanding the fix |
| **COMPILATION_FIX_SUMMARY.md** | Visual diagrams & quick reference | Need diagrams |
| **REGISTER_OPERATIONS_EXPLAINED.md** | Deep dive into registers & bits | Learning microcontroller details |
| **ARCHITECTURE_DIAGRAM.md** | System architecture & flowcharts | Understanding data flow |
| **README_IMPLEMENTATION.md** | This file - quick reference | Need overall guidance |

---

## Summary

✅ **Complete voltage detection system implemented**
✅ **No missing HAL driver dependencies**
✅ **Direct register access for small, fast code**
✅ **10 Hz sampling rate (100ms intervals)**
✅ **12-bit precision (0-4095 range)**
✅ **Voltage range: 0-3.3V**
✅ **Sample tracking for statistics**
✅ **Comprehensive documentation**

---

## Next Steps

1. ✅ Review VOLTAGE_DETECTION_README.md for system overview
2. ✅ Review ARCHITECTURE_DIAGRAM.md to understand data flow
3. ✅ Connect voltage source to PA0
4. ✅ Compile and program the board
5. ✅ Test with known voltages
6. ✅ Extend with filtering or logging as needed

---

## Support

For detailed information on:
- **How it works**: See ARCHITECTURE_DIAGRAM.md
- **Register details**: See REGISTER_OPERATIONS_EXPLAINED.md
- **What changed**: See IMPLEMENTATION_FIX_DETAILS.md
- **System setup**: See VOLTAGE_DETECTION_README.md

All documentation is in the project root directory.

---

**Status**: ✅ Ready to compile and deploy
**Last Updated**: March 9, 2026
**Implementation**: Direct register-based ADC (no HAL driver needed)
