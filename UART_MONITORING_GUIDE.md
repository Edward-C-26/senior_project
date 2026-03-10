# UART Monitoring - View Voltage in Terminal

## Current Status: No Terminal Output Yet

Your voltage detection code works, but voltage readings are **only in RAM**. They're not printed to terminal.

---

## Solution: Add UART Output

### Option 1: Quick Method (Printf via ST-LINK) - EASIEST

With STM32CubeIDE, you can use **printf redirection** to see output in IDE console.

#### Step 1: Add Printf Code to main.c

In your main loop, replace:
```c
// BEFORE
while (1)
{
    uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
    float voltage = VoltageDetector_GetVoltage(&voltage_detector);
    VoltageDetector_UpdateSampleCount(&voltage_detector);
    HAL_Delay(100);
}
```

With:
```c
// AFTER - with printf output
while (1)
{
    uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
    float voltage = VoltageDetector_GetVoltage(&voltage_detector);
    VoltageDetector_UpdateSampleCount(&voltage_detector);
    
    // Print to terminal
    printf("Raw: %4lu | Voltage: %.2f V | Samples: %lu\r\n", 
           raw, voltage, voltage_detector.sample_count);
    
    HAL_Delay(100);
}
```

#### Step 2: Enable Semihosting in IDE

1. Right-click project → Properties
2. C/C++ Build → Settings
3. MCU GCC Compiler → Miscellaneous
4. Add: `-specs=rdimon.specs -lc -lm -lrdimon`

#### Step 3: Configure Debugger

1. Run → Debug Configurations
2. Your debug config → Debugger tab
3. GDB Client Setup:
   - Add line: `handle SIGPIPE noprint nostop`
   - Add line: `define hook-quit`
   - Add line: `set confirm off`
   - Add line: `end`

#### Step 4: Flash & View Output

1. Flash with Debug (Cmd+F11)
2. Console will show output:
   ```
   Raw:    0 | Voltage: 0.00 V | Samples: 1
   Raw: 2048 | Voltage: 1.65 V | Samples: 2
   Raw: 4095 | Voltage: 3.30 V | Samples: 3
   ```

**Advantage**: No extra hardware needed, output in IDE console  
**Disadvantage**: Only works with debugger connected

---

### Option 2: UART via USB (RECOMMENDED) - More Professional

Use UART to send data to your computer's serial terminal.

#### Step 1: Enable UART in CubeMX

1. Open CubeMX (right-click project → Open in CubeMX)
2. Pinout & Configuration
3. Connectivity → USART2
4. Mode: Asynchronous
5. Generate Code (Alt+K)

#### Step 2: Add UART Code to main.c

At top of file, add:
```c
#include <stdio.h>

// Redirect printf to UART
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

In main loop:
```c
while (1)
{
    uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
    float voltage = VoltageDetector_GetVoltage(&voltage_detector);
    VoltageDetector_UpdateSampleCount(&voltage_detector);
    
    // Print voltage data
    printf("Voltage: %.2f V (Raw: %lu)\r\n", voltage, raw);
    
    HAL_Delay(100);
}
```

#### Step 3: Flash Code

1. Rebuild project (includes new UART code)
2. Flash with Run (not Debug)

#### Step 4: Connect Serial Terminal

**On macOS:**
```bash
# Find port
ls /dev/tty.usbserial*
ls /dev/tty.SLAB_USBtoUART*

# Connect (replace with your port)
screen /dev/tty.usbserial 115200

# To exit: Ctrl+A, then Ctrl+D
```

**On Linux:**
```bash
# Find port
ls /dev/ttyUSB*
ls /dev/ttyACM*

# Connect (replace with your port)
screen /dev/ttyUSB0 115200
```

**On Windows:**
```
• Use PuTTY
• Select "Serial"
• Port: COM3 (or whatever shows in Device Manager)
• Speed: 115200
• Click "Open"
```

**Alternative (easier): Using Python**
```bash
pip install pyserial
python -m serial.tools.list_ports  # Find port
python << 'EOF'
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
while True:
    line = ser.readline().decode().strip()
    if line:
        print(line)
EOF
```

#### Example Output:
```
Voltage: 0.00 V (Raw: 0)
Voltage: 1.65 V (Raw: 2048)
Voltage: 3.30 V (Raw: 4095)
Voltage: 2.47 V (Raw: 3072)
Voltage: 0.82 V (Raw: 1024)
```

**Advantage**: Professional, works without debugger  
**Disadvantage**: Requires UART configuration in CubeMX

---

## Comparison

| Method | Setup | Works Without Debugger | Hardware Needed |
|--------|-------|------------------------|-----------------|
| Printf via ST-LINK | Easy (1 step) | ❌ No | ST-LINK (included) |
| UART Serial | Medium (3 steps) | ✅ Yes | UART driver |

---

## Quick Choice Guide

**Choose Option 1 (Printf) if:**
- You want quick debugging
- You're okay with debugger running
- You want simplest setup

**Choose Option 2 (UART) if:**
- You want production-like monitoring
- You want to leave device running standalone
- You have serial terminal software

---

## What to Print

### Basic Output
```c
printf("Voltage: %.2f V\r\n", voltage_detector.voltage);
```

### Detailed Output
```c
printf("Raw: %lu | V: %.3f | Samples: %lu\r\n", 
       voltage_detector.raw_value,
       voltage_detector.voltage,
       voltage_detector.sample_count);
```

### CSV Format (for logging)
```c
printf("%lu,%f,%lu\r\n", 
       voltage_detector.raw_value,
       voltage_detector.voltage,
       voltage_detector.sample_count);
```

### With Timestamp
```c
static uint32_t timestamp = 0;
printf("[%5lu] V=%.2fV Raw=%lu\r\n", 
       timestamp++,
       voltage_detector.voltage,
       voltage_detector.raw_value);
```

---

## Recommended Setup

1. **For Development**: Use Option 1 (Printf via ST-LINK)
   - Fastest to implement
   - View in IDE console
   - Perfect for debugging

2. **For Testing**: Use Option 2 (UART Serial)
   - More professional
   - Works standalone
   - Can save logs to file

---

## Next Steps

**To implement immediately:**

1. Add printf code to main.c
2. For Option 1: Add flags to compiler, enable semihosting in debugger
3. For Option 2: Enable UART in CubeMX, regenerate code, add UART redirection
4. Flash and monitor terminal output

---

## Summary

**Right now**: Voltage is read and stored but not printed  
**After Option 1**: See voltage in IDE console while debugging  
**After Option 2**: See voltage in serial terminal, anytime, anywhere

**Choose one and implement - takes 5 minutes!**

