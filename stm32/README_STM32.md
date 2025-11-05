# 4x4 Matrix Keypad Driver for STM32

Fast, non-blocking matrix keypad driver for STM32 microcontrollers using HAL library. Compatible with F4, G0, and most other STM32 series.

## 📦 Two Versions Available

### 🟢 Simple Version
**Files:** `matrix_stm32.h/c`  
**Best for:** Learning, prototyping, simple projects  
**Features:** Basic polling-based scanning, easy to understand

### 🛡️ Robust Version (NEW!)
**Files:** `matrix_robust_stm32.h/c`  
**Best for:** Production, safety-critical, power-sensitive applications  
**Features:**
- ⏱️ Hardware timer (TIM2/TIM3) for precise scanning
- 📦 Event queue (32 events, thread-safe)
- 🚫 Ghost key detection
- ⚠️ Stuck key detection
- 💤 Power management (STOP mode + EXTI wake)
- 📊 Statistics & monitoring
- 🔒 FreeRTOS compatible

📖 **[Read ROBUST_STM32.md for setup guide](ROBUST_STM32.md)** ⭐

## Hardware Compatibility

✅ **Tested/Designed for:**
- Nucleo-F401RE (Cortex-M4, 84MHz)
- Nucleo-G070RB / G071RB (Cortex-M0+, 64MHz)
- Should work with any STM32 series (F0, F1, F3, F4, F7, H7, L0, L4, G0, G4, etc.)

## Features

✅ **Non-blocking** - No delays, perfect for real-time applications  
✅ **Fast scanning** - Microsecond-precision timing using DWT cycle counter  
✅ **Software debouncing** - Eliminates false triggers  
✅ **HAL-based** - Works with STM32CubeMX generated projects  
✅ **Pin tester** - Identifies your keypad's pin mapping  
✅ **Function mode** - F key toggles special function mode  
✅ **Flexible GPIO** - Use any GPIO pins on any port  

## Quick Start

### 1. Create STM32CubeMX Project

1. Open STM32CubeMX
2. Select your board (e.g., Nucleo-F401RE)
3. **Enable USART2** (for printf debugging):
   - Connectivity → USART2 → Mode: Asynchronous
   - 115200 baud, 8N1
4. **DON'T configure GPIO pins** - the driver does this automatically
5. **Make sure GPIO clocks are enabled** for the ports you'll use
6. Generate code for your IDE

### 2. Add Driver Files to Project

Copy these files to your project `Core/Src` and `Core/Inc` folders:

**Core/Inc:**
- `matrix_stm32.h`
- `keymap_functions_stm32.h`

**Core/Src:**
- `matrix_stm32.c`
- `keymap_functions_stm32.c`

### 3. Configure Pins in Your main.c

Replace the main function with code from:
- `main_example_f401re.c` (for F4 series)
- `main_example_g0.c` (for G0 series)

**Update the pin definitions to match your wiring:**

```c
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOA, 0),   // Row 0 -> PA0
    MAKE_PIN(GPIOA, 1),   // Row 1 -> PA1
    MAKE_PIN(GPIOA, 4),   // Row 2 -> PA4
    MAKE_PIN(GPIOA, 5)    // Row 3 -> PA5
};

const GPIO_Pin_t col_pins[4] = {
    MAKE_PIN(GPIOB, 0),   // Col 0 -> PB0
    MAKE_PIN(GPIOB, 1),   // Col 1 -> PB1
    MAKE_PIN(GPIOB, 4),   // Col 2 -> PB4
    MAKE_PIN(GPIOB, 5)    // Col 3 -> PB5
};
```

### 4. Enable printf (Optional but Recommended)

Add to your `main.c` (before main function):

```c
#ifdef __GNUC__
int _write(int fd, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}
#endif
```

Also ensure in your project settings:
- Use newlib-nano (reduces code size)
- Enable "Use float with printf" if needed

### 5. Build and Flash

1. Build the project
2. Flash to your STM32 board
3. Connect serial terminal (115200 baud)
4. Press keys!

## Hardware Wiring

### Connection Diagram

```
Keypad          STM32
------          -----
Row 0    →      GPIO (output, e.g., PA0)
Row 1    →      GPIO (output, e.g., PA1)
Row 2    →      GPIO (output, e.g., PA4)
Row 3    →      GPIO (output, e.g., PA5)

Col 0    →      GPIO (input, e.g., PB0)
Col 1    →      GPIO (input, e.g., PB1)
Col 2    →      GPIO (input, e.g., PB4)
Col 3    →      GPIO (input, e.g., PB5)
```

**Notes:**
- Rows are configured as **outputs** (push-pull, high-speed)
- Columns are configured as **inputs with internal pull-ups**
- **No external resistors needed** - uses internal pull-ups
- Use any available GPIO pins

### Recommended Pins by Board

#### Nucleo-F401RE
- **Rows:** PA0, PA1, PA4, PA5
- **Cols:** PB0, PB1, PB4, PB5
- These are on CN7/CN10 headers

#### Nucleo-G0 Series
- **Rows:** PC0, PC1, PC2, PC3
- **Cols:** PC4, PC5, PC6, PC7
- All on one port for easy wiring

## Usage Examples

### Basic Key Reading

```c
KeyEvent event;

while (1) {
    if (matrix_scan(&event)) {
        if (event.state == KEY_PRESSED) {
            printf("Key pressed: 0x%X\n", event.key);
        }
    }
}
```

### With Function Mode

```c
keymap_init();  // Enable function mode

while (1) {
    if (matrix_scan(&event)) {
        if (event.state == KEY_PRESSED) {
            bool handled = keymap_process_key(event.key);
            
            if (!handled) {
                // Normal mode
                printf("Key: 0x%X\n", event.key);
            }
        }
    }
}
```

Press **F** to toggle between normal and function mode.

### Blocking Key Read (Simple)

```c
uint8_t key = matrix_get_key();
if (key != 0) {
    printf("Key: 0x%X\n", key);
}
```

### Timer-Based Scanning (Advanced)

For best performance, call `matrix_scan()` from a timer interrupt (e.g., 1kHz):

```c
// In your timer ISR
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    static KeyEvent event;
    if (matrix_scan(&event)) {
        // Handle event in main loop using a queue/flag
    }
}
```

## Pin Testing Mode

If unsure about your keypad wiring:

1. Set `PIN_TEST_MODE 1` in main.c
2. Build and flash
3. Open serial terminal
4. Press each key - see which row/col index is triggered
5. Update pin arrays if needed
6. Set `PIN_TEST_MODE 0` and rebuild

## Performance

- **Scan rate:** ~2.5kHz full keypad scan (no delay)
- **Response time:** < 500μs typical
- **Debounce:** 3 scans (~1.2ms at 2.5kHz rate)
- **CPU usage:** < 1% (most time spent sleeping)

Adjust scan frequency in main loop:
- No delay: Maximum speed (~2.5kHz)
- `HAL_Delay(1)`: 1kHz scan rate (recommended)
- Call from timer: Precise timing

## Customizing Function Mode

Edit `keymap_functions_stm32.c` to customize function actions:

```c
void function_key_1(uint8_t key) {
    printf("  -> Toggle LED\n");
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // Example
}
```

See `FUNCTION_MODE.md` in the parent directory for more details.

## Troubleshooting

### No keys detected?
- **Check GPIO clock:** Ensure `__HAL_RCC_GPIOx_CLK_ENABLE()` is called
- **Verify wiring:** Use a multimeter to confirm connections
- **Try pin test mode:** Helps identify wiring issues
- **Check pull-ups:** Driver uses internal pull-ups (no external needed)

### False triggers / ghost keys?
- Increase `DEBOUNCE_PRESS` in `matrix_stm32.h`
- Add small capacitors (100nF) across keypad columns
- Check for loose connections

### Printf not working?
- Verify USART2 is configured and initialized
- Add `_write()` function for newlib
- Check baud rate (115200)
- Try ITM/SWO trace instead

### Slow response?
- Remove `HAL_Delay()` from main loop
- Use DWT cycle counter (already implemented)
- Call from timer interrupt for consistent timing

### Compilation errors?
- Ensure `main.h` includes HAL headers
- Add files to project (right-click → Add files)
- Check include paths in project settings

## Advanced Features

### Custom Keymap

Change the key mapping:

```c
const uint8_t my_keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
matrix_set_keymap(my_keymap);
```

### Key Release Detection

```c
if (event.state == KEY_RELEASED) {
    printf("Key released: 0x%X\n", event.key);
}
```

### Long Press Detection

```c
if (event.state == KEY_HELD) {
    // Key is being held down
}
```

## Porting to Other MCUs

The STM32 version uses:
- `HAL_GPIO_*` functions (easy to adapt to LL or SPL)
- `DWT` cycle counter for microsecond delays
- Standard C (no C++ or STL dependencies)

To port to another platform:
1. Replace GPIO functions
2. Replace delay_us() function
3. Update pin structure

## Files Overview

| File | Purpose |
|------|---------|
| `matrix_stm32.h` | Matrix driver API |
| `matrix_stm32.c` | Matrix driver implementation |
| `keymap_functions_stm32.h` | Function mode API |
| `keymap_functions_stm32.c` | Function mode implementation |
| `main_example_f401re.c` | Example for F4 series |
| `main_example_g0.c` | Example for G0 series |

## License

Free to use for any purpose.

## Support

For issues or questions, check the main README.md or FUNCTION_MODE.md.

