# Testing the Robust Matrix Keypad Driver

## 🧪 Complete Testing Guide

### Quick Start

Your `CMakeLists.txt` is now configured to build the robust version!

## Step 1: Build

```bash
cd /Users/ssnow/Documents/GitHub/matrix
cd build
cmake ..
make
```

You should see:
```
[100%] Built target matrix_keypad
```

The output file: `build/matrix_keypad.uf2`

## Step 2: Flash to Pico2

1. **Hold BOOTSEL button** on Pico2
2. **Plug in USB** (while holding BOOTSEL)
3. **Release BOOTSEL** - Pico appears as USB drive
4. **Drag & drop** `matrix_keypad.uf2` to the Pico drive
5. Pico reboots automatically

## Step 3: Connect Serial Monitor

### Option A: Using screen (macOS/Linux)
```bash
screen /dev/tty.usbmodem* 115200
```

### Option B: Using minicom
```bash
minicom -D /dev/tty.usbmodem* -b 115200
```

### Option C: Arduino IDE Serial Monitor
- Tools → Serial Monitor
- Set to 115200 baud

You should see:
```
=== ROBUST Matrix Keypad Driver ===
Hardware timer + interrupts + error detection

Robust matrix keypad initialized (scan rate: 1000 Hz)
Keymap function system initialized.
Press F to toggle function mode.
Scanning started

Scanning active. Features enabled:
  - Hardware timer-based scanning (1kHz)
  - Thread-safe event queue
  - Ghost key detection
  - Stuck key detection (5s timeout)
  - Low power mode support

Press keys or type 'help' for commands
```

---

## 🧪 Test Suite

### Test 1: Basic Key Presses ✅

**What to do:**
- Press keys: 1, 2, 3, A, 4, 5, etc.

**Expected output:**
```
[1234 ms] Key: 0x1 (row=0, col=0)
[1456 ms] Key: 0x2 (row=0, col=1)
[1678 ms] Key: 0x3 (row=0, col=2)
[1890 ms] Key: 0xA (row=0, col=3)
```

**What it tests:**
- Timer interrupt scanning
- Event queue
- Debouncing
- Timestamp accuracy

---

### Test 2: Function Mode (F-Key Toggle) ✅

**What to do:**
1. Press **F** key
2. Press **1**
3. Press **F** again
4. Press **1**

**Expected output:**
```
>>> FUNCTION MODE ACTIVATED <<<
Press 1-E to trigger functions, F to exit.

[Function Mode] Executing function for key 0x1...
  -> FUNCTION 1: Toggle LED / Output Pin

>>> NORMAL MODE <<<

Key: 0x1 (row=0, col=0)
```

**What it tests:**
- Function mode integration
- Mode state management

---

### Test 3: Ghost Key Detection 🚫

**What to do:**
1. Press and hold **1**
2. Press and hold **2** (keep 1 held)
3. Press and hold **4** (keep 1 and 2 held)
4. Press **5** (while all others held)

**Expected output:**
```
Key: 0x1 (row=0, col=0)
Key: 0x2 (row=0, col=1)
Key: 0x4 (row=1, col=0)
⚠️  ERROR [5678 ms]: Ghost key detected (row=1, col=1)
```

**What it tests:**
- Ghost key detection algorithm
- Error queue
- Error event generation

---

### Test 4: Stuck Key Detection ⚠️

**What to do:**
1. Press and **HOLD** any key for **6+ seconds**
2. Don't release!

**Expected output:**
```
Key: 0x5 (row=1, col=1)
[after 5 seconds...]
⚠️  ERROR [5000 ms]: Stuck key detected (row=1, col=1)
```

**What it tests:**
- Stuck key timeout
- Long press monitoring
- Error detection

---

### Test 5: Rapid Key Presses 🔥

**What to do:**
- Press keys **as fast as you can** for 10 seconds
- Try to press 10+ keys per second

**Expected output:**
```
[1000 ms] Key: 0x1 (row=0, col=0)
[1050 ms] Key: 0x2 (row=0, col=1)
[1100 ms] Key: 0x3 (row=0, col=2)
[1150 ms] Key: 0xA (row=0, col=3)
[1200 ms] Key: 0x4 (row=1, col=0)
... (all keys registered)
```

**Watch statistics:**
```
--- Statistics ---
Total scans:     10000
Total events:    50      ← All your keypresses
Queue overflows: 0      ← Should be ZERO!
```

**What it tests:**
- Event queue capacity
- No queue overflows
- High-speed handling

---

### Test 6: Statistics Monitoring 📊

**What to do:**
- Wait 10 seconds (statistics print automatically)

**Expected output:**
```
--- Statistics ---
Total scans:     10000
Total events:    15
Total errors:    0
Queue overflows: 0
Max scan time:   12 us
Avg scan time:   8 us
------------------
```

**What it tests:**
- Statistics tracking
- Performance monitoring
- Scan timing

**What to look for:**
- ✅ `Max scan time` < 50μs (good performance)
- ✅ `Queue overflows` = 0 (no lost events)
- ✅ `Total scans` increasing (timer running)

---

### Test 7: Low Power Mode 💤

**What to do:**
1. **Wait 30 seconds** without pressing any keys
2. Watch for sleep message
3. Press **ANY** key to wake

**Expected output:**
```
[After 30 seconds of idle...]
Entering low power mode...
Woke up from key press!
Key: 0x1 (row=0, col=0)
```

**What it tests:**
- Idle detection
- Low power entry
- Wake interrupt
- Resume scanning

**Note:** The `__wfi()` instruction puts the CPU to sleep.

---

### Test 8: Multiple Simultaneous Keys 🎹

**What to do:**
1. Press **1** and **2** simultaneously
2. Press **4** and **5** simultaneously
3. Release all

**Expected output:**
```
Key: 0x1 (row=0, col=0)
Key: 0x2 (row=0, col=1)
Key: 0x4 (row=1, col=0)
Key: 0x5 (row=1, col=1)
[releases...]
Released: 0x1
Released: 0x2
Released: 0x4
Released: 0x5
```

**What it tests:**
- Multi-key detection
- Event ordering
- Release tracking

---

### Test 9: Event Queue Stress Test 🔨

**What to do:**
1. Press **10-15 keys very rapidly** (< 1 second)
2. Check if any were lost

**Expected output:**
```
[All keys registered]
Queue overflows: 0  ← Check in next statistics
```

**What it tests:**
- Queue capacity (32 events)
- ISR performance
- No data loss

---

### Test 10: Release Events 👇

**What to do:**
- Watch the `main_robust_example.c` - release events are commented out by default
- You can see them by uncommenting lines 64-67

**Current (releases hidden):**
```c
// else if (event.state == KEY_RELEASED) {
//     printf("Released: 0x%X\n", event.key);
// }
```

**Uncomment to see:**
```
Key: 0x1 (row=0, col=0)
Released: 0x1
Key: 0x2 (row=0, col=1)
Released: 0x2
```

**What it tests:**
- Release detection
- Full key lifecycle

---

## 🎯 What Should Work

### ✅ Expected Behavior

1. **Keys respond instantly** (< 2ms latency)
2. **No false triggers** (debouncing works)
3. **Ghost keys detected** (error event generated)
4. **Stuck keys detected** (after 5 seconds)
5. **Statistics accurate** (scan count increases)
6. **Queue never overflows** (32 events enough)
7. **Sleep mode works** (30s idle timeout)
8. **Wake works** (any key press)
9. **Function mode toggles** (F key)
10. **No crashes** (stable operation)

---

## ⚠️ Troubleshooting

### Issue: No output in serial monitor

**Check:**
1. Correct baud rate (115200)
2. USB cable connected
3. Correct port selected
4. Wait 2 seconds after reset (USB init time)

**Fix:**
```bash
# List ports
ls /dev/tty.usbmodem*

# Reconnect
screen /dev/tty.usbmodem14101 115200
```

---

### Issue: Keys not detected

**Check:**
1. Keypad wiring (rows to GPIO 2-5, cols to 6-9)
2. Physical connections
3. Keypad orientation (rows vs cols)

**Debug:**
Set `PIN_TEST_MODE 1` to verify wiring first.

---

### Issue: Queue overflows > 0

**Meaning:** Events generated faster than processed

**Fix:**
1. Process events faster (remove printf delays)
2. Increase `EVENT_QUEUE_SIZE` in `matrix_robust.h`
3. Use ISR callbacks instead of queue

---

### Issue: High scan times (> 50μs)

**Meaning:** ISR taking too long

**Check:**
1. Electrical noise
2. Other interrupts interfering
3. USB printf delays

**Fix:**
- Remove printf from hot paths
- Check wiring
- Add capacitors (100nF) to columns

---

### Issue: Ghost keys trigger incorrectly

**Meaning:** Ghost detection too sensitive

**Fix:**
1. Disable ghost detection temporarily:
   ```c
   matrix_robust_set_ghost_detection(false);
   ```
2. Or adjust algorithm in `matrix_robust.c`

---

### Issue: Sleep not working

**Check:**
1. Idle timeout (30 seconds default)
2. Events being generated (keeps resetting timer)

**Fix:**
- Reduce `idle_count` threshold (line 99 in `main_robust_example.c`)

---

## 📊 Performance Benchmarks

### Target Metrics (Pico2 @ 150MHz)

| Metric | Target | Typical |
|--------|--------|---------|
| Max scan time | < 50μs | 10-15μs |
| Avg scan time | < 20μs | 8-10μs |
| Queue overflows | 0 | 0 |
| Response latency | < 2ms | 1-1.5ms |
| CPU usage | < 2% | ~1% |

If your values are in these ranges, **everything is working perfectly!** ✅

---

## 🔧 Advanced Testing

### Test with Your Own Code

Replace the keypress handler in `main_robust_example.c`:

```c
if (event.state == KEY_PRESSED) {
    // YOUR CODE HERE
    switch (event.key) {
        case 0x1:
            // Handle key 1
            break;
        case 0x2:
            // Handle key 2
            break;
        // ... etc
    }
}
```

### Test with RTOS (FreeRTOS)

If you're using FreeRTOS:

```c
void KeypadTask(void *pvParameters) {
    KeyEvent event;
    for (;;) {
        if (matrix_robust_get_event(&event)) {
            process_key(event.key);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

## ✅ Checklist

- [ ] Build successful
- [ ] Flash successful
- [ ] Serial output visible
- [ ] Basic keys work
- [ ] Function mode toggles
- [ ] Ghost detection triggers (3+ keys)
- [ ] Stuck key detection (5+ sec hold)
- [ ] Rapid presses work (10+ per sec)
- [ ] Statistics display
- [ ] No queue overflows
- [ ] Sleep mode works (30s idle)
- [ ] Wake on keypress works
- [ ] Performance good (< 50μs scans)

**All checked?** Your robust driver is working perfectly! 🎉

---

## 🚀 Next Steps

1. ✅ Test completed successfully
2. Customize for your application
3. Integrate with your project
4. Deploy with confidence!

---

## Need Help?

- Check statistics for clues
- Review error messages
- Test each feature individually
- Check troubleshooting section above

**Happy Testing!** 🧪

