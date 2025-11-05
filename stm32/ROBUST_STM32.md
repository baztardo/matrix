# Robust Matrix Keypad for STM32 - Setup Guide

## 🛡️ Overview

The **robust STM32 version** adds hardware timer interrupts, error detection, and power management. Perfect for production deployment on STM32 microcontrollers.

## 🚀 Quick Start

### 1. Create STM32CubeMX Project

Open STM32CubeMX and configure:

#### A. **Select Your Board**
- Nucleo-F401RE, G070RB, or any STM32

#### B. **Enable USART2** (for printf)
- Connectivity → USART2
- Mode: Asynchronous
- Baud: 115200

#### C. **Enable Timer** (for scanning)
- Timers → TIM2 (or TIM3 for G0)
- Mode: Clock Source = Internal Clock
- Configuration:
  - **Prescaler:** Calculate for 1kHz tick
  - **Counter Period:** 1000-1 (for 1kHz interrupt)
  - **Enable global interrupt:** ✅ YES

**Example for 84MHz clock (F401RE):**
- Prescaler: 83 (84MHz / 84 = 1MHz)
- Counter Period: 999 (1MHz / 1000 = 1kHz)

**Example for 64MHz clock (G0):**
- Prescaler: 63 (64MHz / 64 = 1MHz)
- Counter Period: 999 (1MHz / 1000 = 1kHz)

#### D. **Enable GPIO Clocks**
Make sure GPIO clocks are enabled for your chosen ports.

#### E. **Generate Code**
- Project Manager → Generate Code
- Open in your IDE

### 2. Add Driver Files

Copy to your project:

**Core/Inc:**
- `matrix_robust_stm32.h`
- `keymap_functions_stm32.h`

**Core/Src:**
- `matrix_robust_stm32.c`
- `keymap_functions_stm32.c`

### 3. Update main.c

Replace your `main()` with code from:
- `main_robust_example_f401re.c` (F4 series)
- `main_robust_example_g0.c` (G0 series)

**Key changes:**
```c
// Add extern for timer
extern TIM_HandleTypeDef htim2;  // Or htim3

// In main()
matrix_robust_init(row_pins, col_pins, &htim2, 1000);
matrix_robust_set_ghost_detection(true);
matrix_robust_set_stuck_detection(true, 5000);
matrix_robust_start();

// Add callbacks outside main()
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    matrix_robust_timer_callback(htim);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    matrix_robust_exti_callback(GPIO_Pin);
}
```

### 4. Build and Flash

Build → Flash → Test!

---

## 🔧 Configuration Details

### Timer Configuration

The timer **MUST** generate interrupts at your desired scan rate:

| Scan Rate | Interrupt Frequency | Example Use |
|-----------|---------------------|-------------|
| 100Hz | Every 10ms | Very low power |
| 500Hz | Every 2ms | Normal use |
| 1kHz | Every 1ms | **Recommended** |
| 2kHz | Every 500μs | High performance |

**Formula:**
```
Timer Frequency = Clock / (Prescaler + 1)
Interrupt Rate = Timer Frequency / (Period + 1)
```

**Example (F401RE @ 84MHz, want 1kHz):**
```
Timer Freq = 84MHz / 84 = 1MHz
Interrupt = 1MHz / 1000 = 1kHz ✓
```

Set in CubeMX:
- Prescaler: 83
- Counter Period (ARR): 999

### Pin Configuration

Choose your GPIO pins and update in `main.c`:

```c
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOA, 0),   // PA0
    MAKE_PIN(GPIOA, 1),   // PA1
    MAKE_PIN(GPIOA, 4),   // PA4
    MAKE_PIN(GPIOA, 5)    // PA5
};

const GPIO_Pin_t col_pins[4] = {
    MAKE_PIN(GPIOB, 0),   // PB0
    MAKE_PIN(GPIOB, 1),   // PB1
    MAKE_PIN(GPIOB, 4),   // PB4
    MAKE_PIN(GPIOB, 5)    // PB5
};
```

See [PIN_SELECTION_GUIDE.md](PIN_SELECTION_GUIDE.md) for recommendations.

---

## 🛡️ Features

### 1. Hardware Timer Scanning

**Benefit:** Precise, consistent timing
- No polling jitter
- Main loop can be busy
- ISR-driven

**How it works:**
- Timer interrupt every 1ms (configurable)
- Scans one row per interrupt
- Full scan = 4ms (all 4 rows)

### 2. Event Queue

**Benefit:** Never miss keypresses
- 32-event circular buffer
- Thread-safe with critical sections
- Timestamps included

**Usage:**
```c
KeyEvent event;
while (matrix_robust_get_event(&event)) {
    printf("Key: 0x%X at %lu ms\n", event.key, event.timestamp);
}
```

### 3. Ghost Key Detection

**Benefit:** Prevents false multi-key triggers

**Enable:**
```c
matrix_robust_set_ghost_detection(true);
```

**Handle:**
```c
ErrorEvent error;
if (matrix_robust_get_error(&error)) {
    if (error.error_code == ERROR_GHOST_KEY) {
        printf("Ghost key at row=%d, col=%d\n", error.row, error.col);
    }
}
```

### 4. Stuck Key Detection

**Benefit:** Identifies mechanical failures

**Enable:**
```c
matrix_robust_set_stuck_detection(true, 5000);  // 5 second timeout
```

### 5. Power Management

**Benefit:** Battery life optimization

**Enter low power:**
```c
matrix_robust_enter_low_power();
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
SystemClock_Config();  // Reconfigure after wake
```

**Wake on keypress:**
- Columns configured as EXTI
- Any key press triggers interrupt
- System wakes automatically

### 6. Statistics

**Monitor performance:**
```c
ScanStatistics stats;
matrix_robust_get_statistics(&stats);

printf("Scans: %lu\n", stats.total_scans);
printf("Events: %lu\n", stats.total_events);
printf("Errors: %lu\n", stats.total_errors);
printf("Max scan time: %lu us\n", stats.max_scan_time_us);
```

---

## 🔌 Integration with CubeMX

### Method 1: Direct Integration (Recommended)

1. CubeMX generates code
2. Copy driver files
3. Modify `main.c` in USER CODE sections
4. Add callbacks

### Method 2: Keep main.c Clean

Don't modify generated `main.c`. Instead:

1. Create `app_keypad.c` / `app_keypad.h`
2. Call `app_keypad_init()` from `main.c`
3. Put all keypad code in `app_keypad.*`

---

## 🧪 Testing

### Basic Test
1. Build and flash
2. Open serial terminal (115200 baud)
3. Press keys
4. See events in queue

### Test Ghost Detection
1. Press keys: [1], [2]
2. Press keys: [4], [5]
3. Should detect potential ghost

### Test Stuck Key
1. Hold any key for 5+ seconds
2. Error event generated

### Test Power Management
1. Wait 30 seconds (idle timeout)
2. System enters sleep
3. Press any key
4. System wakes

---

## 🔧 Advanced Configuration

### Change Scan Rate

In `matrix_robust_init()`:
```c
matrix_robust_init(pins, pins, &htim2, 2000);  // 2kHz scan rate
```

Adjust timer in CubeMX accordingly.

### Custom Debounce Times

Edit in `matrix_robust_stm32.h`:
```c
#define DEBOUNCE_PRESS_MS   20   // Press debounce
#define DEBOUNCE_RELEASE_MS 50   // Release debounce
```

### Larger Event Queue

Edit in `matrix_robust_stm32.h`:
```c
#define EVENT_QUEUE_SIZE 64   // 64 events instead of 32
```

### ISR Callbacks (Real-Time)

For immediate response:
```c
void on_key_press(KeyEvent *event) {
    // Called from ISR - keep FAST!
    if (event->key == 0xE) {
        HAL_GPIO_WritePin(EMERGENCY_STOP_GPIO_Port, EMERGENCY_STOP_Pin, GPIO_PIN_SET);
    }
}

matrix_robust_set_key_callback(on_key_press);
```

**⚠️ ISR Safety:**
- No `printf()`
- No `HAL_Delay()`
- No blocking calls
- Keep execution < 10μs

---

## 🔀 FreeRTOS Integration

The robust version is **FreeRTOS-ready**!

```c
// Task 1: Process keys
void KeypadTask(void *argument)
{
  KeyEvent event;
  for(;;)
  {
    if (matrix_robust_get_event(&event)) {
      process_keypress(event.key);
    }
    osDelay(10);
  }
}

// Task 2: Monitor errors
void ErrorTask(void *argument)
{
  ErrorEvent error;
  for(;;)
  {
    if (matrix_robust_get_error(&error)) {
      log_error(&error);
    }
    osDelay(100);
  }
}
```

**Thread safety:** All queue operations use critical sections.

---

## 📊 Performance

**Measured on F401RE @ 84MHz:**

| Metric | Value |
|--------|-------|
| Single row scan | ~10μs |
| Full scan (4 rows) | ~40μs |
| Event enqueue | ~2μs |
| CPU usage @ 1kHz | < 1% |

**G0 @ 64MHz is similar!**

---

## 🐛 Troubleshooting

### Timer interrupt not firing

**Check:**
1. TIM global interrupt enabled in NVIC (CubeMX)
2. `HAL_TIM_Base_Start_IT()` called (done by `matrix_robust_start()`)
3. Prescaler/Period calculated correctly
4. Timer clock enabled

**Debug:**
Add breakpoint in `HAL_TIM_PeriodElapsedCallback()`

### Queue overflows

**Symptom:** `queue_overflows > 0` in statistics

**Fix:**
1. Increase `EVENT_QUEUE_SIZE`
2. Process events faster
3. Use ISR callbacks

### EXTI wake not working

**Check:**
1. Column pins configured as EXTI inputs
2. EXTI interrupt enabled in NVIC
3. Correct GPIO_Pin passed to callback

### High scan times

**Symptom:** `max_scan_time_us > 100`

**Causes:**
- Other high-priority interrupts
- Electrical noise
- Poor connections

**Fix:**
- Prioritize timer interrupt
- Add capacitors to columns
- Check wiring

---

## 📝 CubeMX Configuration Checklist

- [ ] Board selected
- [ ] USART2 enabled (115200 baud)
- [ ] TIM2/TIM3 configured (1kHz interrupt)
- [ ] TIM global interrupt enabled
- [ ] GPIO clocks enabled
- [ ] Code generated
- [ ] Driver files copied
- [ ] main.c modified
- [ ] Callbacks added
- [ ] Printf redirection added
- [ ] Build successful
- [ ] Test passed

---

## 🎯 Next Steps

1. ✅ Configure timer in CubeMX
2. ✅ Copy driver files
3. ✅ Update main.c
4. ✅ Build and flash
5. ✅ Test all features
6. ✅ Enable error detection
7. ✅ Test power management
8. ✅ Monitor statistics
9. ✅ Deploy!

---

## 📚 See Also

- **[ROBUST_FEATURES.md](../ROBUST_FEATURES.md)** - Detailed feature guide (Pico version, but concepts apply)
- **[README_STM32.md](README_STM32.md)** - General STM32 setup
- **[PIN_SELECTION_GUIDE.md](PIN_SELECTION_GUIDE.md)** - Choose the right pins

## License

Free to use for any purpose.

