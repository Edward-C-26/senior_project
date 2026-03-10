# Voltage Detection Implementation - Fix Details

## Problem
The initial implementation used the STM32 HAL ADC driver (`stm32f4xx_hal_adc.h`), which was not included in the project's Drivers folder. This caused a compilation error:

```
fatal error: stm32f4xx_hal_adc.h: No such file or directory
```

## Solution
Instead of relying on the missing HAL ADC driver, we implemented voltage detection using **direct register access**. This is a lower-level approach that works directly with the microcontroller's hardware registers.

---

## Changes Made

### 1. **stm32f4xx_hal_conf.h** - Disabled ADC Module

**Before:**
```c
#define HAL_ADC_MODULE_ENABLED
```

**After:**
```c
/* #define HAL_ADC_MODULE_ENABLED */
```

**Why:** Since the ADC HAL driver files don't exist in the project, we disabled it to avoid compilation errors. The ADC functionality is now handled directly via register access.

---

### 2. **voltage_detector.h** - Simplified Structure

**Before:**
```c
typedef struct {
    ADC_HandleTypeDef hadc;      // HAL-specific structure
    uint32_t raw_value;
    float voltage;
    uint32_t sample_count;
} VoltageDetector_t;
```

**After:**
```c
typedef struct {
    uint32_t raw_value;
    float voltage;
    uint32_t sample_count;
} VoltageDetector_t;
```

**Why:** Removed `ADC_HandleTypeDef hadc` since we're not using HAL functions anymore. The structure now only stores measurement data.

**Other changes:**
```c
#define ADC_INSTANCE             ((ADC_TypeDef *)ADC1_BASE)
```

**Why:** Changed from HAL's ADC enumeration to a direct register-based reference.

---

### 3. **voltage_detector.c** - Complete Rewrite Using Register Access

#### **Register Definitions Added:**

```c
/* STM32F401 Register definitions */
#define RCC_APB2ENR  (*(volatile uint32_t *)(0x40023800 + 0x44))
#define RCC_AHB1ENR  (*(volatile uint32_t *)(0x40023800 + 0x30))

#define ADC1_CR2     (*(volatile uint32_t *)(0x40012000 + 0x08))
#define ADC1_SMPR2   (*(volatile uint32_t *)(0x40012000 + 0x10))
#define ADC1_SQR3    (*(volatile uint32_t *)(0x40012000 + 0x34))
#define ADC1_DR      (*(volatile uint32_t *)(0x40012000 + 0x4C))

#define GPIOA_MODER  (*(volatile uint32_t *)(0x40020000 + 0x00))
```

**Explanation:**
- `0x40023800` = RCC (Reset and Clock Control) base address
- `0x40012000` = ADC1 base address
- `0x40020000` = GPIOA base address
- `volatile` = Prevents compiler optimization; tells it to read from actual register
- Offsets are added to get specific register addresses

---

#### **Function: VoltageDetector_Init() - Refactored**

**New implementation:**

```c
void VoltageDetector_Init(VoltageDetector_t *detector)
{
    /* Enable GPIOA and ADC1 clocks */
    RCC_AHB1ENR |= (1 << 0);  /* Enable GPIOA clock */
    RCC_APB2ENR |= (1 << 8);  /* Enable ADC1 clock */
```

**What it does:**
- `RCC_AHB1ENR |= (1 << 0)` = Sets bit 0 of RCC_AHB1ENR to enable GPIOA clock
- `RCC_APB2ENR |= (1 << 8)` = Sets bit 8 of RCC_APB2ENR to enable ADC1 clock

```c
    /* Configure PA0 as analog input */
    GPIOA_MODER &= ~(3 << 0);  /* Clear PA0 mode bits */
    GPIOA_MODER |= (3 << 0);   /* Set PA0 to analog mode (11) */
```

**What it does:**
- `GPIOA_MODER` controls the mode of each GPIO pin
- Each pin has 2 bits: 00=Digital Input, 01=Output, 10=Alternate, 11=Analog
- `&= ~(3 << 0)` clears bits 0-1
- `|= (3 << 0)` sets bits 0-1 to binary 11 (analog mode)

```c
    /* Configure ADC1 */
    ADC1_SMPR2 &= ~(7 << 0);    /* Clear channel 0 sampling bits */
    ADC1_SMPR2 |= (6 << 0);     /* Set to 84 cycles (110 = 84 cycles) */
```

**What it does:**
- `ADC1_SMPR2` = Sampling time register for channels 0-9
- `(7 << 0)` clears 3 bits for channel 0
- `(6 << 0)` sets to binary 110 = 84 clock cycles sampling time

```c
    /* Set channel 0 in regular sequence */
    ADC1_SQR3 &= ~(0x1F << 0);  /* Clear channel selection bits */
    ADC1_SQR3 |= (0 << 0);       /* Select channel 0 */
```

**What it does:**
- `ADC1_SQR3` = Regular sequence register (which channels to convert)
- Clears 5 bits and sets channel 0 as the first conversion

```c
    /* Enable ADC1 */
    ADC1_CR2 |= (1 << 0);  /* ADON: ADC ON */
```

**What it does:**
- `ADC1_CR2` = Control register 2
- Bit 0 (ADON) = ADC On. Setting it turns on the ADC

---

#### **Function: VoltageDetector_GetRawValue() - Refactored**

**New implementation:**

```c
uint32_t VoltageDetector_GetRawValue(VoltageDetector_t *detector)
{
    /* Start conversion */
    ADC1_CR2 |= (1 << 30);  /* SWSTART: Start conversion */
    
    /* Wait for conversion to complete (EOC flag) */
    uint32_t timeout = 100000;
    while (!(ADC1_DR & (1 << 31)) && timeout--);
```

**What it does:**
- `ADC1_CR2 |= (1 << 30)` = Sets SWSTART bit to start software conversion
- Loop waits for EOC (End Of Conversion) flag
- `ADC1_DR & (1 << 31)` checks if bit 31 is set (EOC flag)
- `timeout--` prevents infinite loop if ADC fails

```c
    /* Read the converted value */
    detector->raw_value = ADC1_DR & 0xFFF;  /* Mask to 12 bits */
    
    /* Calculate voltage */
    detector->voltage = detector->raw_value * VOLTAGE_SCALE;
    
    return detector->raw_value;
}
```

**What it does:**
- Reads the data register and masks to 12 bits
- Same voltage calculation as before
- Returns the raw ADC value

---

## Why Direct Register Access?

| Aspect | HAL Approach | Direct Register Approach |
|--------|-------------|-------------------------|
| **File dependency** | Requires `stm32f4xx_hal_adc.h` | Only needs hardware knowledge |
| **Code size** | Larger (hundreds of functions) | Smaller and more direct |
| **Speed** | Minimal overhead | Fastest possible |
| **Flexibility** | Limited to HAL design | Full hardware control |
| **Learning value** | Black-box abstraction | Understanding microcontroller deeply |

---

## Register Reference (STM32F401)

### RCC_AHB1ENR (0x40023830)
- Bit 0: GPIOAEN = GPIO Port A clock enable

### RCC_APB2ENR (0x40023844)
- Bit 8: ADC1EN = ADC1 clock enable

### GPIOA_MODER (0x40020000)
- Bits [1:0] = PA0 mode (11 = analog)

### ADC1_CR2 (0x40012008)
- Bit 0: ADON = ADC on
- Bit 30: SWSTART = Start conversion by software

### ADC1_SMPR2 (0x40012010)
- Bits [2:0] = Channel 0 sampling time (110 = 84 cycles)

### ADC1_SQR3 (0x40012034)
- Bits [4:0] = First conversion in sequence (channel 0)

### ADC1_DR (0x4001204C)
- Bits [11:0] = Converted data
- Bit 31: EOC = End of conversion flag

---

## How It Works Now

```
PA0 receives analog voltage
         ↓
ADC hardware converts (84 cycles for accuracy)
         ↓
Result stored in ADC1_DR register
         ↓
EOC flag set (bit 31 becomes 1)
         ↓
VoltageDetector_GetRawValue() reads ADC1_DR
         ↓
Masks to 12 bits (removes EOC flag)
         ↓
Calculates: raw × 3.3/4095
         ↓
Returns voltage value
```

---

## Key Differences from HAL Implementation

1. **No HAL dependency** - Works without HAL ADC driver
2. **Direct bit manipulation** - Uses bitwise operators (&, |, <<, >>)
3. **Lower abstraction** - Directly controls hardware registers
4. **Smaller code** - Only the functions needed
5. **Faster** - No function call overhead

---

## Testing the Implementation

```c
// In main loop
uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
float voltage = VoltageDetector_GetVoltage(&voltage_detector);
VoltageDetector_UpdateSampleCount(&voltage_detector);

// Example results with 1.65V input:
// raw = 2048 (approximately)
// voltage = 1.65V (approximately)
// After 10 seconds: sample_count = 100
```

---

## Advantages of This Approach

✓ No external dependencies  
✓ Smaller binary size  
✓ Compile without missing files  
✓ Direct hardware control  
✓ Educational value  

---

## If You Want to Add HAL Later

To restore HAL-based ADC functionality:
1. Download STM32F4xx HAL drivers from STMicroelectronics
2. Copy ADC driver files to `Drivers/STM32F4xx_HAL_Driver/Inc/`
3. Re-enable `#define HAL_ADC_MODULE_ENABLED` in HAL config
4. Restore original voltage_detector.c (using HAL_ADC functions)

---

## Files Modified

1. ✓ `Core/Inc/stm32f4xx_hal_conf.h` - Disabled ADC module
2. ✓ `Core/Inc/voltage_detector.h` - Removed HAL dependency
3. ✓ `Core/Src/voltage_detector.c` - Rewritten with register access
4. ✓ `Core/Inc/main.h` - No changes needed
5. ✓ `Core/Src/main.c` - No changes needed
