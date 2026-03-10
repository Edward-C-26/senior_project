# Build & Flash Checklist

## ⚡ QUICK START (5 minutes)

### 1. Build
```
STM32CubeIDE: Project → Build Project (or Cmd+B)
```
✓ No errors? Continue to step 2

### 2. Connect
```
Plug Nucleo F401 into USB
Check: lsusb | grep STM
```
✓ Device appears? Continue to step 3

### 3. Flash  
```
STM32CubeIDE: Run → Debug (or Cmd+F11)
Wait for: "Download completed successfully"
```
✓ Message appears? Code is running! ✅

---

## 📋 DETAILED CHECKLIST

### Pre-Build
- [ ] STM32CubeIDE installed
- [ ] voltage_detector.h in Core/Inc/
- [ ] voltage_detector.c in Core/Src/
- [ ] main.c includes voltage_detector.h
- [ ] No obvious syntax errors in IDE

### Build
- [ ] Project → Clean Project
- [ ] Project → Build Project
- [ ] Build completes without errors
- [ ] BreadDemo.elf exists in Debug folder
- [ ] Console shows "100% Built target"

### Hardware
- [ ] Nucleo F401 board available
- [ ] USB cable ready
- [ ] Computer USB port available
- [ ] No USB devices already using port

### Connect
- [ ] Plug USB into Nucleo board
- [ ] Plug USB into computer
- [ ] Nucleo power LED lights up (red)
- [ ] Wait 2-3 seconds for drivers
- [ ] Run: lsusb | grep STM (shows device)

### Configure Flash (First Time Only)
- [ ] Run → Debug Configurations
- [ ] Create new "STM32 C/C++ Application"
- [ ] Set Project: BreadDemo
- [ ] Set C/C++ Application: BreadDemo.elf
- [ ] Set Debugger: ST-LINK
- [ ] Set Interface: SWD
- [ ] Click Apply and Close

### Flash Code
- [ ] Run → Debug (or Cmd+F11)
- [ ] Console shows build progress
- [ ] Console shows "Downloading..."
- [ ] Console shows "Download completed"
- [ ] No error messages
- [ ] Device resets automatically

### Verify Success
- [ ] "Download completed successfully" message
- [ ] Nucleo LED still on (not blinking in error pattern)
- [ ] IDE console shows no errors
- [ ] Device doesn't disconnect

### Test (Optional)
- [ ] Connect 0V to PA0 (GND)
- [ ] Watch voltage_detector.voltage ≈ 0V
- [ ] Connect 3.3V to PA0
- [ ] Watch voltage_detector.voltage ≈ 3.3V
- [ ] Values update every 100ms

---

## 🎯 NO IOC CHANGES NEEDED!

Your voltage detection code is self-contained in:
- Core/Inc/voltage_detector.h ✅
- Core/Src/voltage_detector.c ✅

**You do NOT need to modify BreadDemo.ioc**

Just compile and flash! ✅

---

## ⚠️ If Something Goes Wrong

### Build Error
```
→ Project → Clean
→ Project → Build Project
```

### Connection Error  
```
→ Disconnect/reconnect USB
→ Check: lsusb | grep STM
→ Install: brew install stlink
```

### Flash Error
```
→ Press RESET button on Nucleo
→ Try Run → Debug again
```

### Still stuck?
→ See: BUILD_AND_FLASH_GUIDE.md (Detailed troubleshooting)

---

## ✅ SUCCESS INDICATORS

All of these should be true:
- ✅ Code compiled without errors
- ✅ "Download completed" message appears
- ✅ Nucleo board LED is on
- ✅ No "device disconnected" message
- ✅ Code runs (doesn't crash/hang)

**If all ✅, you're done!**

---

## 📱 Your Code is Running

When you see "Download completed", your board is running:
```
main()
  → VoltageDetector_Init()    (Set up PA0)
  → VoltageDetector_Start()   (Enable ADC)
  → while(1) loop
    - Read voltage from PA0
    - Calculate 0-3.3V value
    - Update sample counter
    - Wait 100ms
```

Connect a voltage source (0-3.3V) to PA0 and it will continuously measure it!

---

## 🚀 You're All Set!

Build → Flash → Done!

No IOC modifications needed. No complex settings. Just compile and go.

Enjoy your voltage detection system! 🎉

