# STM32F401 Build & Flash Guide

## Quick Summary

Your project is ready to compile and flash. The **STM32CubeIDE** (or standard IDE setup) will handle most settings automatically.

---

## Step 1: Build the Project

### Using STM32CubeIDE

1. **Open Project**
   - File → Open Projects from File System
   - Select: `/Users/edward41803/Documents/gitlab/BreadDemo`
   - Click "Finish"

2. **Build Project**
   - Right-click project → Build Project
   - Or: Project → Build Project
   - Or: Keyboard shortcut (usually Ctrl+B or Cmd+B)

3. **Expected Output**
   ```
   Building file: Core/Src/voltage_detector.c
   Building file: Core/Src/main.c
   Linking Nucleo_F401_Demo.elf
   [100%] Built target Nucleo_F401_Demo
   ```

### Build Configuration

Your project uses:
- **Toolchain**: ARM GCC (arm-none-eabi-gcc)
- **Linker Scripts**: 
  - `STM32F401RETX_FLASH.ld` (for Flash memory)
  - `STM32F401RETX_RAM.ld` (for RAM execution)
- **Optimization**: Depends on build configuration (usually `-O0` for Debug, `-O2` for Release)

---

## Step 2: Configure Debug/Flash Settings

### In STM32CubeIDE

1. **Open Run Configurations**
   - Run → Run Configurations
   - Or: Run → Debug Configurations

2. **Create Configuration (if needed)**
   - Right-click "STM32 C/C++ Application" → New
   - Name: `BreadDemo Flash`
   - Project: Select your project
   - C/C++ Application: `BreadDemo.elf`
   - Build configuration: `Release` (or `Debug`)

3. **Set Debugger**
   - Tab: "Debugger"
   - Debugger: `ST-LINK`
   - Driver: `ST-LINK (Nucleo board)`
   - Interface: `SWD` (Serial Wire Debug)
   - Port: `/dev/ttyACM0` (or auto-detect)

4. **Set Flash Settings**
   - Tab: "Startup"
   - Check: ☑ "Load symbols"
   - Check: ☑ "Set program counter at main"
   - Check: ☑ "Resume"
   - Flash method: "Flash memory"

---

## Step 3: Flash/Program the Device

### Option A: Using IDE (Easiest)

1. **Connect Board**
   - Plug Nucleo F401 into USB port
   - LED should light up (power indicator)
   - Check connection: `lsusb | grep STM`

2. **Flash via IDE**
   - Run → Run (for quick flash)
   - Or: Run → Debug (for debugging)
   - Wait for "Download completed successfully"

### Option B: Using Terminal (Alternative)

```bash
# 1. Navigate to project
cd /Users/edward41803/Documents/gitlab/BreadDemo

# 2. Build
make clean
make all

# 3. Flash using st-link
st-flash write BreadDemo.bin 0x08000000

# Or using openocd
openocd -f /usr/share/openocd/scripts/board/st_nucleo_f4.cfg \
  -c "program BreadDemo.elf verify reset exit"
```

---

## Step 4: Verify Flash Success

### Check in IDE
- Message: "Download completed successfully"
- Console shows: `Verifying... OK`
- Device resets automatically

### Manual Verification
After programming:
1. Nucleo LED blinks (normal operation)
2. USB connection active
3. No error messages

---

## IOC File Configuration (CubeMX)

Your `BreadDemo.ioc` is the STM32CubeMX configuration file. 

### Current Configuration

The IOC file contains:
- ✅ System clock setup (HSI 16 MHz)
- ✅ GPIO configurations
- ✅ Debug/SWD pins enabled
- ✅ UART setup (if present)

### To Regenerate Code from IOC

If you modify `.ioc` in CubeMX:

1. **Open CubeMX**
   - Right-click project → Open in CubeMX
   - Or: Open `BreadDemo.ioc` directly

2. **Edit Settings**
   - Change clock, peripherals, pins as needed

3. **Generate Code**
   - Project → Generate Code (Alt+K)
   - Click "Yes" to overwrite
   - **Note**: This will regenerate HAL files but NOT touch your voltage_detector files

4. **Rebuild Project**
   - Your voltage_detector.c/h are preserved
   - Main project rebuilds with new HAL setup

### Important Notes for IOC

**DO NOT change in CubeMX:**
- ❌ Do NOT disable HSI clock (system needs it)
- ❌ Do NOT remove SWD pins (needed for debugging)
- ❌ Do NOT remove GPIO Port A (PA0 is used)

**Safe to change:**
- ✅ Clock frequency (if needed)
- ✅ Add additional peripherals (UART, etc.)
- ✅ Configure unused pins

---

## Build Configuration Details

### Debug Build (for development)
```
Optimization: -O0 (no optimization)
Debug symbols: -g3
Smaller size: No
```
- Slower execution
- Easier to debug
- Larger binary

### Release Build (for deployment)
```
Optimization: -O2 or -O3 (optimize for speed/size)
Debug symbols: None (or stripped)
Smaller size: Yes
```
- Faster execution
- Harder to debug
- Smaller binary

### Switch Configurations
- **In IDE**: Project → Build Configuration → Set Active
- **In Makefile**: `make clean && make all DEBUG=0`

---

## Troubleshooting Flash Issues

### Error: "No ST-LINK detected"

**Solution 1: Check Connection**
```bash
# List USB devices
lsusb | grep STM

# Should show: STMicroelectronics ST-LINK/V2
```

**Solution 2: Install ST-LINK Drivers**
```bash
# macOS (using Homebrew)
brew install stlink

# Linux (Ubuntu/Debian)
sudo apt-get install stlink-tools
```

**Solution 3: Reset USB Connection**
- Unplug Nucleo board
- Wait 5 seconds
- Plug back in
- Try again

### Error: "Cannot download code"

**Solution 1: Rebuild Project**
- Clean: Project → Clean
- Build: Project → Build

**Solution 2: Check Linker Script**
- Verify `STM32F401RETX_FLASH.ld` exists
- Check Flash size: 512 KB (correct for F401)
- Check RAM size: 96 KB (correct for F401)

**Solution 3: Reset Board**
- Press RESET button on Nucleo (black button)
- Try flashing again

### Error: "Verification failed"

**Solution: Retry with Erase First**
- Run → Run Configurations
- Debugger tab → GDB Client Setup
- Add option: `-c "monitor erase_mass"`

---

## Typical Complete Workflow

### First Time Setup

```
1. Open project in STM32CubeIDE
   ↓
2. Right-click → Build Project
   ↓
3. Connect Nucleo board via USB
   ↓
4. Run → Debug Configurations
   ↓
5. Create new ST-LINK configuration
   ↓
6. Click "Debug" button
   ↓
7. Code downloads and executes
```

### Normal Development Cycle

```
1. Make code changes in main.c or voltage_detector.c
   ↓
2. Cmd+B (or Project → Build)
   ↓
3. Cmd+F11 (or Run → Debug) to flash and debug
   ↓
4. Set breakpoints and step through code
   ↓
5. Monitor variables in IDE
```

### Quick Flash (without debugging)

```
1. Build: Project → Build Project
   ↓
2. Flash: Run → Run (or Cmd+F11)
   ↓
3. Code executes immediately
```

---

## Useful IDE Shortcuts

| Action | macOS | Windows/Linux |
|--------|-------|---------------|
| Build | Cmd+B | Ctrl+B |
| Debug | Cmd+F11 | F11 |
| Run | Cmd+F11 | Ctrl+F11 |
| Stop | Cmd+F2 | Ctrl+F2 |
| Step Over | Cmd+F6 | F6 |
| Step Into | Cmd+F5 | F5 |
| Resume | Cmd+F8 | F8 |
| Clean | Cmd+Shift+B | - |

---

## For Your Voltage Detection Code

### What Happens When You Flash

1. **Compilation**
   - `voltage_detector.c` compiled to object code
   - `main.c` compiled to object code
   - Linked together into `BreadDemo.elf`

2. **Flashing**
   - `BreadDemo.elf` converted to binary
   - Downloaded to Flash memory at `0x08000000`
   - Device resets
   - Code starts executing from main()

3. **Execution**
   - HAL_Init() called
   - SystemClock_Config() sets up clock
   - VoltageDetector_Init() configures PA0 and ADC
   - VoltageDetector_Start() enables ADC
   - Main loop runs continuously
   - Voltage read every 100ms

### No Additional IOC Changes Needed

Your voltage detection code is fully implemented in C files, not CubeMX-generated code. You do **NOT** need to:
- Configure anything in CubeMX for the ADC
- Regenerate HAL code
- Modify any IOC settings

The code is ready to compile and flash as-is!

---

## Debugging with Breakpoints

### Set Breakpoint in main loop

1. Click in line number column next to:
   ```c
   uint32_t raw = VoltageDetector_GetRawValue(&voltage_detector);
   ```

2. Double-click to toggle breakpoint (red dot appears)

3. Run Debug (Cmd+F11)

4. Program pauses at breakpoint

5. **Inspect Variables**
   - Windows → Show View → Variables
   - Watch: `voltage_detector.voltage`
   - Watch: `voltage_detector.raw_value`

6. **Step through code**
   - F5 = Step Into function
   - F6 = Step Over function
   - F8 = Resume execution

---

## Summary: Step-by-Step for Your Board

### 1. Build
```
Project → Build Project
(or Cmd+B)
```

### 2. Connect
```
Plug Nucleo F401 into USB
Wait for ST-LINK driver to load
```

### 3. Create Flash Configuration (first time only)
```
Run → Debug Configurations
Create "STM32 C/C++ Application"
Set debugger to ST-LINK/SWD
```

### 4. Flash Code
```
Run → Debug
(or Cmd+F11)
Wait for "Download completed"
```

### 5. Monitor Voltage
```
Connect 0-3.3V source to PA0
Watch voltage_detector.voltage in debugger
```

---

## That's It!

Your voltage detection system is now running on the Nucleo F401 board. The ADC will continuously read PA0 and update the voltage value every 100ms.

**No IOC modifications needed** - your implementation is self-contained in the C files!

Happy coding! 🚀

