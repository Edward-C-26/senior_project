# STM32F401 Voltage Detection System

## Overview
This project implements a voltage detection machine using the STM32F401 Nucleo board. It detects and continuously reads voltage from an analog input and tracks the sample count.

## Hardware Setup

### Components Required
- **STM32F401 Nucleo Board**
- **Analog Input Source**: Connect voltage to **PA0** (ADC1 Channel 0)
- **Reference Voltage**: 3.3V (internal VRef)
- **ADC Resolution**: 12-bit

### Pin Configuration
- **PA0**: Analog Input for Voltage Detection
  - ADC1 Channel 0
  - Input range: 0 - 3.3V

## Software Implementation

### Files Added/Modified

1. **Core/Inc/voltage_detector.h**
   - Defines VoltageDetector_t structure
   - ADC channel and GPIO configuration
   - Function prototypes for voltage measurement

2. **Core/Src/voltage_detector.c**
   - ADC initialization and configuration
   - Voltage reading functions
   - Sample counting functionality

3. **Core/Src/main.c**
   - Initializes voltage detector at startup
   - Continuous voltage reading loop with 100ms sampling interval
   - Tracks sample count

4. **Core/Inc/stm32f4xx_hal_conf.h**
   - Enabled HAL_ADC_MODULE for ADC functionality

### Key Functions

#### VoltageDetector_Init(VoltageDetector_t *detector)
- Configures GPIO PA0 as analog input
- Initializes ADC1 with 12-bit resolution
- Sets continuous conversion mode

#### VoltageDetector_Start(VoltageDetector_t *detector)
- Starts ADC conversion

#### VoltageDetector_GetRawValue(VoltageDetector_t *detector)
- Polls ADC for converted value
- Returns raw 12-bit ADC value (0-4095)
- Internally calculates voltage

#### VoltageDetector_GetVoltage(VoltageDetector_t *detector)
- Returns calculated voltage in volts
- Range: 0 - 3.3V

#### VoltageDetector_UpdateSampleCount(VoltageDetector_t *detector)
- Increments sample counter for statistics

## Voltage Calculation

```
ADC_Value = 0 - 4095 (12-bit)
Voltage = ADC_Value × (3.3V / 4095)
```

### Example
- ADC reading = 2048 → Voltage = 1.65V
- ADC reading = 4095 → Voltage = 3.3V
- ADC reading = 0 → Voltage = 0V

## Sampling Details

- **Sample Rate**: 10 Hz (100ms interval)
- **ADC Clock**: PCLK2 / 4
- **Sampling Time**: 84 clock cycles per sample
- **Continuous Mode**: Enabled for automatic re-conversion

## Usage Example

```c
// Voltage detector is initialized and running in main()
// Readings are taken every 100ms in the main loop

// Retrieve voltage:
uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
float voltage = VoltageDetector_GetVoltage(&voltage_detector);

// Sample count tracks total measurements:
uint32_t total_samples = voltage_detector.sample_count;
```

## Extending the System

### To add UART logging:
1. Enable UART module in HAL config
2. Add printf-like output for voltage values
3. Send results to external monitoring system

### To increase sample rate:
- Reduce HAL_Delay() in main loop
- Adjust ADC_SAMPLETIME_84CYCLES for faster conversion

### To add filtering:
- Implement moving average filter
- Store last N samples for averaging
- Reduces noise in readings

## Testing

Monitor the voltage detector structure:
- `voltage_detector.raw_value` - Raw 12-bit ADC value
- `voltage_detector.voltage` - Calculated voltage (volts)
- `voltage_detector.sample_count` - Total samples collected

## Troubleshooting

**No voltage readings?**
- Verify PA0 is connected to voltage source
- Check ADC clock enable in HAL config
- Confirm 3.3V reference supply

**Inconsistent readings?**
- Increase sampling time in ADC config
- Add external filtering capacitor on PA0
- Check for noise in voltage source
