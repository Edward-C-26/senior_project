# Register-Based ADC Implementation - Deep Dive

## What Are Registers?

Registers are special memory locations in the microcontroller that directly control hardware. When you write to a register, you're telling the hardware to do something. When you read from a register, you're getting the current hardware state.

```
┌──────────────────────────────────┐
│  Your Code (C code in main.c)    │
└──────────────┬───────────────────┘
               │ Reads/Writes
               ▼
┌──────────────────────────────────┐
│  Register (Hardware Address)     │
│  Example: 0x40023830            │
└──────────────┬───────────────────┘
               │ Triggers
               ▼
┌──────────────────────────────────┐
│  Hardware (GPIO, ADC, etc)       │
└──────────────────────────────────┘
```

---

## Register Syntax in C

### Creating a Register Reference

```c
#define RCC_AHB1ENR  (*(volatile uint32_t *)(0x40023800 + 0x30))
```

**Breaking this down:**

```
RCC_AHB1ENR          → Name of this register
(*(volatile uint32_t *)(...))  → Dereference pointer (means: treat this address as data)
volatile             → Don't optimize this; it changes independently
uint32_t             → 32-bit unsigned integer
0x40023800 + 0x30    → Address 0x40023830 in memory
```

**In plain English:**
> "RCC_AHB1ENR is a 32-bit value stored at memory address 0x40023830. It can change on its own, so read it fresh every time."

### Example: Enable GPIO Clock

```c
RCC_AHB1ENR |= (1 << 0);
```

**Step-by-step execution:**

1. **Read**: Get current value of RCC_AHB1ENR from address 0x40023830
   ```
   Current value: 0x00000000 (binary: 00000000000000000000000000000000)
   ```

2. **Create mask**: `(1 << 0)` means shift 1 left 0 times
   ```
   1 in binary: 00000000000000000000000000000001
   Shift left 0: 00000000000000000000000000000001 (no change)
   ```

3. **Bitwise OR**: Combine current value with mask
   ```
   0x00000000 | 0x00000001 = 0x00000001
   
   Before: 00000000000000000000000000000000
   Mask:   00000000000000000000000000000001
   After:  00000000000000000000000000000001
   ```

4. **Write**: Store result back to register
   ```
   Hardware detects bit 0 changed from 0 to 1
   Action: "Enable GPIOA clock!" ✓
   ```

---

## Bit Operations Reference

### Bit Shifting

```c
(1 << 0)    // Binary: 00000001 (bit 0)
(1 << 1)    // Binary: 00000010 (bit 1)
(1 << 2)    // Binary: 00000100 (bit 2)
(1 << 3)    // Binary: 00001000 (bit 3)
(1 << 8)    // Binary: 100000000 (bit 8)
(1 << 31)   // Binary: 10000000000000000000000000000000 (bit 31)
```

### Bitwise OR (Set bit)

```c
x |= (1 << n)   // Set bit n to 1
```

**Example: Set bit 0**
```
Before: 00000000
OR with: 00000001
After:  00000001
```

### Bitwise AND with NOT (Clear bit)

```c
x &= ~(1 << n)  // Set bit n to 0
```

**Example: Clear bit 0**
```
Before: 00000001
AND with: 11111110 (NOT of 00000001)
After:  00000000
```

### Bitwise AND (Check bit)

```c
if (x & (1 << n))   // Check if bit n is 1
```

**Example: Check bit 0**
```
Value:   00000001
AND with: 00000001
Result:  00000001 (non-zero = true)
```

### Multi-bit mask

```c
(7 << 0)     // Binary: 00000111 (3 bits, positions 0-2)
(0x1F << 0)  // Binary: 00011111 (5 bits, positions 0-4)
(3 << 0)     // Binary: 00000011 (2 bits, positions 0-1)
```

---

## Register Operations Explained

### Operation 1: Enable GPIO Clock

```c
RCC_AHB1ENR |= (1 << 0);
```

| Step | Value | Meaning |
|------|-------|---------|
| Register | 0x40023830 | RCC_AHB1ENR address |
| Bit to set | Bit 0 | GPIOA clock enable bit |
| Mask | 0x00000001 | Only bit 0 |
| Operation | OR | Keep all bits, SET bit 0 |
| Result | Bit 0 = 1 | GPIOA clock enabled ✓ |

**Real-world effect:**
```
GPIOA now receives clock signal from RCC
GPIOA pins can now be configured
```

---

### Operation 2: Enable ADC Clock

```c
RCC_APB2ENR |= (1 << 8);
```

| Step | Value | Meaning |
|------|-------|---------|
| Register | 0x40023844 | RCC_APB2ENR address |
| Bit to set | Bit 8 | ADC1 clock enable bit |
| Mask | 0x00000100 | Only bit 8 (binary: 100000000) |
| Operation | OR | Keep all bits, SET bit 8 |
| Result | Bit 8 = 1 | ADC1 clock enabled ✓ |

**Real-world effect:**
```
ADC1 now receives clock signal from RCC
ADC1 can now convert analog signals
```

---

### Operation 3: Configure PA0 as Analog

```c
GPIOA_MODER &= ~(3 << 0);  // Clear bits 0-1
GPIOA_MODER |= (3 << 0);   // Set bits 0-1 to 11
```

**What GPIOA_MODER controls:**

```
Bits [1:0] for PA0:
00 = Digital input
01 = Digital output
10 = Alternate function
11 = Analog input ← We want this
```

**Step 1: Clear existing mode**
```c
GPIOA_MODER &= ~(3 << 0);
// ~(3 << 0) = ~(11₂) = ...11111100₂
// AND operation clears bits [1:0]

Before: XX...XX 11 XX
Clear:  XX...XX 00 XX (bits 0-1 now 0)
```

**Step 2: Set to analog mode**
```c
GPIOA_MODER |= (3 << 0);
// (3 << 0) = 11₂
// OR operation sets bits [1:0] to 11

Before: XX...XX 00 XX
Set:    XX...XX 11 XX (bits 0-1 now 1)
```

**Real-world effect:**
```
PA0 pin is now in analog mode
Can now read analog voltage from PA0
Digital logic disabled
```

---

### Operation 4: Set ADC Sampling Time

```c
ADC1_SMPR2 &= ~(7 << 0);    // Clear bits 0-2
ADC1_SMPR2 |= (6 << 0);     // Set to 110 (84 cycles)
```

**What the bits mean:**

```
ADC1_SMPR2 bits [2:0] for channel 0:
000 = 3 cycles
001 = 15 cycles
010 = 28 cycles
011 = 56 cycles
100 = 84 cycles
101 = 112 cycles
110 = 144 cycles
111 = 480 cycles

We choose: 110 = 84 cycles (good balance of speed and accuracy)
```

**Step 1: Clear existing value**
```c
ADC1_SMPR2 &= ~(7 << 0);
// Clears bits [2:0]

Before: XX...XX 111 XX
Clear:  XX...XX 000 XX
```

**Step 2: Set to 84 cycles**
```c
ADC1_SMPR2 |= (6 << 0);
// 6 in binary is 110
// Sets bits [2:0] to 110

Before: XX...XX 000 XX
Set:    XX...XX 110 XX (bits = 110)
```

**Real-world effect:**
```
Each ADC sample now takes 84 clock cycles
More time = More accurate reading
Slower = Trade-off for precision
```

---

### Operation 5: Start ADC Conversion

```c
ADC1_CR2 |= (1 << 30);
```

**What bit 30 does:**

```
ADC1_CR2[30] = SWSTART (Software Start Conversion)
0 = No action
1 = Start conversion
```

**Execution:**
```c
ADC1_CR2 |= (1 << 30);
// Sets bit 30 to 1

Before: bit 30 = 0
Action: ADC starts converting analog input
After:  Conversion in progress...
```

**Real-world effect:**
```
ADC starts measuring PA0
Samples the voltage for 84 cycles
Converts to digital value (0-4095)
Stores result in ADC1_DR register
Sets EOC flag when done
```

---

### Operation 6: Wait for Conversion Complete

```c
uint32_t timeout = 100000;
while (!(ADC1_DR & (1 << 31)) && timeout--);
```

**Breaking it down:**

```c
ADC1_DR & (1 << 31)  // Read register and check bit 31
```

**What happens:**
1. `ADC1_DR` = Read data register at 0x4001204C
2. `(1 << 31)` = Create mask for bit 31 (EOC flag)
3. `& (1 << 31)` = Check if bit 31 is set
4. `!(...)` = Negate (true if bit 31 is 0)
5. `while (...)` = Keep looping while bit 31 is still 0

**Conversion timeline:**
```
Time:   0ms ──────────── 10μs ───── 20μs
Event:  START          END      EOC=1
        [Converting...] [Done] [Flag set]
        bit 31 = 0      bit 31 = 1
```

**Timeout mechanism:**
```c
timeout--           // Decrement timeout counter
while (...  && timeout--)  // Stop if timeout reaches 0
```

If conversion takes too long (>100000 iterations), the loop exits to prevent infinite hang.

**Real-world effect:**
```
Waits for ADC to finish converting
Detects when bit 31 (EOC flag) is set
Exits loop when conversion complete
Prevents program from hanging forever
```

---

### Operation 7: Read and Mask ADC Result

```c
detector->raw_value = ADC1_DR & 0xFFF;
```

**What it does:**

```
ADC1_DR             = 32-bit register
bits [11:0]         = Actual ADC data (12 bits)
bits [31:12]        = Status flags and reserved
0xFFF               = Hex for binary 111111111111 (12 ones)
& 0xFFF             = AND with mask to extract only bits [11:0]
```

**Example:**
```
Raw register value: 0x800001AB
In binary:         10000000000000000001101010110
Mask 0xFFF:        00000000000000000000111111111111
AND result:        00000000000000000000000110101011
Decimal:           2048 + 171 = 2219
```

**Real-world effect:**
```
ADC result is in bits [11:0]
Masking removes status flags
Keeps only the 12-bit conversion value
Range: 0 to 4095
```

---

### Operation 8: Calculate Voltage

```c
detector->voltage = detector->raw_value * VOLTAGE_SCALE;
```

**Where VOLTAGE_SCALE is defined:**

```c
#define VOLTAGE_SCALE (3.3f / 4095.0f)
```

**Calculation:**

```
ADC reading range:  0 ─────────────── 4095
Voltage range:      0V ────────────── 3.3V

VOLTAGE_SCALE = 3.3 ÷ 4095 = 0.000806V/count

Example: raw_value = 2048
voltage = 2048 × 0.000806 = 1.65V
```

**Conversion table:**
| Raw Value | Voltage |
|-----------|---------|
| 0 | 0.000V |
| 1024 | 0.825V |
| 2048 | 1.649V |
| 3072 | 2.474V |
| 4095 | 3.300V |

**Real-world effect:**
```
Converts raw ADC count to actual voltage
Result is in volts (0 to 3.3V)
Float precision allows decimal values
3.3V reference voltage of STM32F401
```

---

## Complete Initialization Sequence

```c
void VoltageDetector_Init(VoltageDetector_t *detector)
{
    // Step 1: Enable clocks (hardware can now be configured)
    RCC_AHB1ENR |= (1 << 0);      // GPIOA clock ON
    RCC_APB2ENR |= (1 << 8);      // ADC1 clock ON
    
    // Step 2: Configure GPIO (PA0 becomes analog input)
    GPIOA_MODER &= ~(3 << 0);     // Clear PA0 mode
    GPIOA_MODER |= (3 << 0);      // Set PA0 to analog (11)
    
    // Step 3: Configure ADC channel (how to sample)
    ADC1_SMPR2 &= ~(7 << 0);      // Clear sampling bits
    ADC1_SMPR2 |= (6 << 0);       // 84 cycles sampling time
    
    // Step 4: Select channel in sequence (which pin to convert)
    ADC1_SQR3 &= ~(0x1F << 0);    // Clear channel bits
    ADC1_SQR3 |= (0 << 0);        // Select channel 0 (PA0)
    
    // Step 5: Turn on ADC (hardware ready to convert)
    ADC1_CR2 |= (1 << 0);         // ADON: ADC enabled
    
    // Step 6: Initialize data structure
    detector->raw_value = 0;
    detector->voltage = 0.0f;
    detector->sample_count = 0;
}
```

**Hardware state after each step:**

```
Step 1: Clocks enabled
        ├─ GPIOA receives clock signal
        └─ ADC1 receives clock signal

Step 2: PA0 configured as input
        ├─ PA0 no longer digital I/O
        └─ PA0 receives analog signal

Step 3: Sampling time set
        ├─ ADC knows to sample for 84 cycles
        └─ Provides accurate readings

Step 4: Channel selected
        ├─ ADC knows to convert PA0 (channel 0)
        └─ Other channels not used

Step 5: ADC enabled
        ├─ ADC operational
        └─ Ready to convert on command

Step 6: Variables initialized
        ├─ raw_value = 0
        ├─ voltage = 0.0V
        └─ sample_count = 0
```

---

## Conversion Sequence (per call)

```c
uint32_t VoltageDetector_GetRawValue(VoltageDetector_t *detector)
{
    // 1. Trigger conversion
    ADC1_CR2 |= (1 << 30);
    
    // 2. Wait for completion
    uint32_t timeout = 100000;
    while (!(ADC1_DR & (1 << 31)) && timeout--);
    
    // 3. Read result
    detector->raw_value = ADC1_DR & 0xFFF;
    
    // 4. Calculate voltage
    detector->voltage = detector->raw_value * VOLTAGE_SCALE;
    
    // 5. Return raw value
    return detector->raw_value;
}
```

**Timeline:**
```
Time  Action                  Hardware State
────  ──────────────────────  ──────────────────────
0μs   ADC1_CR2 |= (1<<30)    SWSTART=1, conversion begins
      └─ bits [11:0] start converting
1μs   Sampling starts        Charging capacitor on PA0
10μs  Sampling complete      Reading analog voltage
20μs  Conversion complete    ADC1_DR has value, EOC=1
25μs  Program reads EOC      !(ADC1_DR & (1<<31)) = 0
      └─ Loop exits
26μs  raw_value = ...        Stores 12-bit result
27μs  voltage = ...          Multiplies by scale factor
28μs  return ...             Value passed to main loop
```

---

## Summary

| Operation | Register | Bits | Result |
|-----------|----------|------|--------|
| Enable GPIO | RCC_AHB1ENR | [0] | GPIOA clock ON |
| Enable ADC | RCC_APB2ENR | [8] | ADC1 clock ON |
| Set PA0 analog | GPIOA_MODER | [1:0]=11 | PA0 accepts analog input |
| Set sample time | ADC1_SMPR2 | [2:0]=110 | 84 cycle sampling |
| Select channel | ADC1_SQR3 | [4:0]=0 | Convert channel 0 (PA0) |
| Enable ADC | ADC1_CR2 | [0] | ADC ready |
| Start convert | ADC1_CR2 | [30] | Begin conversion |
| Check done | ADC1_DR | [31] | EOC flag set when ready |
| Read data | ADC1_DR | [11:0] | 12-bit result (0-4095) |
| Calculate | Math | N/A | voltage = raw × 3.3/4095 |

---

## Advantages of Understanding Registers

✓ **Complete control**: Do exactly what you need  
✓ **Small code**: No bloat from unused features  
✓ **Fast**: No function call overhead  
✓ **Portable**: Works even without HAL driver  
✓ **Educational**: Understand microcontroller deeply  
✓ **Debugging**: Can inspect register states  
✓ **Optimization**: Hand-tune for your needs  

---

## Resources for Further Learning

1. STM32F401 Datasheet (Register definitions)
2. STM32F401 Reference Manual (Register bit descriptions)
3. ARM Cortex-M4 Programming Guide (Memory mapping)
4. C bitwise operators documentation
