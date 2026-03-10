# Quick Flash Reference Card

## TL;DR - Fast Path to Running Code

### 1. Build (2 minutes)
```
STM32CubeIDE:
  Project → Build Project
  
Or terminal:
  cd /Users/edward41803/Documents/gitlab/BreadDemo
  make clean && make all
```

### 2. Connect Hardware (1 minute)
```
• Plug Nucleo F401 into USB
• Wait for LED to light up
• Check: lsusb | grep STM
```

### 3. Flash Code (2 minutes)
```
Option A (Easiest - via IDE):
  Run → Debug (Cmd+F11)
  
Option B (Terminal):
  st-flash write BreadDemo.bin 0x08000000
```

### 4. Verify Success
```
• IDE shows: "Download completed successfully"
• Nucleo LED responds to code
• Code running - voltage reading continuously
```

**Total time: ~5 minutes**

---

## IDE Configuration Summary

### First Time Only
1. Run → Debug Configurations
2. Create new "STM32 C/C++ Application"
3. Set Debugger: ST-LINK / SWD
4. Click "Debug"

### Every Time After
- Just press: **Cmd+F11** (or Run → Debug)
- Code builds, downloads, and runs automatically

---

## What NOT to Change in IOC File

❌ **Do NOT modify** (will break project):
- HSI clock setting
- SWD/debug pin configuration
- GPIO Port A configuration
- Flash memory settings

✅ **Safe to modify** (won't affect voltage detector):
- Additional peripherals
- Clock frequency (if needed)
- Unused pin configurations

**Note:** Voltage detection code does NOT use CubeMX-generated ADC code. It's self-contained in `voltage_detector.c`.

---

## No IOC Changes Needed!

Your voltage detection is implemented entirely in C files:
- `Core/Inc/voltage_detector.h` ← Header
- `Core/Src/voltage_detector.c` ← Implementation

**You DO NOT need to regenerate code from IOC file.**

Just compile and flash - it works as-is! ✅

---

## Troubleshooting

### "Build failed"
```
→ Project → Clean
→ Project → Build Project
```

### "ST-LINK not found"
```
→ Disconnect/reconnect USB
→ Check: lsusb | grep STM
→ Install: brew install stlink (macOS)
```

### "Download failed"
```
→ Press RESET button on Nucleo
→ Try flashing again
→ Or: monitor erase_mass option in debugger
```

---

## Important Files

| File | Purpose | Change? |
|------|---------|---------|
| `BreadDemo.ioc` | CubeMX config | ❌ Not needed |
| `voltage_detector.h` | ADC header | ✅ Already done |
| `voltage_detector.c` | ADC code | ✅ Already done |
| `main.c` | Entry point | ✅ Already done |
| `Makefile` | Build script | ❌ Leave alone |
| `STM32F401RETX_FLASH.ld` | Linker script | ❌ Leave alone |

---

## Success Indicators

After flashing, you should see:
1. ✅ Code compiles with no errors
2. ✅ IDE says "Download completed"
3. ✅ Nucleo LED stays on or blinks
4. ✅ No error messages in console
5. ✅ (Optional) voltage values update every 100ms

**If you see all ✅, you're ready to use it!**

---

## What's Running on Your Board

```
main()
  ↓
HAL_Init() + SystemClock_Config()
  ↓
VoltageDetector_Init()    ← Sets up PA0 as analog input
  ↓
VoltageDetector_Start()   ← Enables ADC
  ↓
while(1) loop
  ├─ Read PA0 voltage
  ├─ Calculate 0-3.3V value
  ├─ Update sample counter
  └─ Wait 100ms
  (repeats forever)
```

Connect 0-3.3V to PA0 → Board continuously reads and measures it!

---

## Quick Commands

```bash
# Build only
make clean && make all

# Build and flash
make clean && make all && st-flash write BreadDemo.bin 0x08000000

# Erase and flash
st-flash erase && st-flash write BreadDemo.bin 0x08000000

# Check connection
lsusb | grep STM

# Monitor serial (if UART enabled)
screen /dev/ttyACM0 115200
```

---

## You're All Set! 🎉

No IOC modifications needed.
No build configuration needed.
Just build and flash!

**Start with:** BUILD_AND_FLASH_GUIDE.md for detailed instructions

