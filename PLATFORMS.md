# Platform Comparison & File Guide

## Quick Platform Selection

| Feature | Pico/Pico2 | STM32 |
|---------|-----------|-------|
| **MCU** | RP2040 / RP2350 | F0/F1/F3/F4/F7/G0/G4/H7/L0/L4 |
| **SDK** | Pico SDK | STM32 HAL |
| **Build System** | CMake | STM32CubeIDE / CMake |
| **Setup Complexity** | Easy | Moderate |
| **Pin Flexibility** | Fixed (or modify) | Fully flexible |
| **Files Needed** | 6 | 6 |

## Which Files to Use?

### 📁 For Raspberry Pi Pico/Pico2

**Core Files:**
```
matrix.h
matrix.c
main.c
```

**Function Mode (Optional):**
```
keymap_functions.h
keymap_functions.c
```

**Build:**
```
CMakeLists.txt
```

**Documentation:**
- `README.md` (this file)
- `FUNCTION_MODE.md`

**Ignore:**
- `stm32/` folder (not needed)

---

### 📁 For STM32 (F4, G0, etc.)

**Core Files:**
```
stm32/matrix_stm32.h
stm32/matrix_stm32.c
```

**Function Mode (Optional):**
```
stm32/keymap_functions_stm32.h
stm32/keymap_functions_stm32.c
```

**Example Code:**
```
stm32/main_example_f401re.c  (for F4 series)
stm32/main_example_g0.c      (for G0 series)
```

**Documentation:**
- `stm32/README_STM32.md` ⭐ **Start here!**
- `stm32/PIN_SELECTION_GUIDE.md`
- `FUNCTION_MODE.md` (in root)

**Build:**
- Use STM32CubeIDE project
- Or integrate into existing CMake project

**Ignore:**
- Root `matrix.c`, `matrix.h` (Pico-specific)
- `CMakeLists.txt` in root (Pico-specific)

---

## Feature Comparison

| Feature | Pico | STM32 | Notes |
|---------|------|-------|-------|
| Non-blocking | ✅ | ✅ | Both use polling |
| Debouncing | ✅ | ✅ | Software, configurable |
| Pull-ups | ✅ | ✅ | Internal pull-ups |
| Pin tester | ✅ | ✅ | Identical functionality |
| Function mode | ✅ | ✅ | F key toggles modes |
| Speed | ~2.5kHz | ~2.5kHz | Adjustable on both |
| μs delays | `sleep_us()` | `DWT` counter | Platform-specific |
| GPIO API | Pico SDK | HAL | Different APIs |
| Setup time | 5 min | 10-15 min | STM32 needs CubeMX |

## Code Portability

The **API is identical** between platforms:

```c
// Initialize (both platforms)
matrix_init(row_pins, col_pins);

// Scan (both platforms)
if (matrix_scan(&event)) {
    if (event.state == KEY_PRESSED) {
        printf("Key: 0x%X\n", event.key);
    }
}

// Function mode (both platforms)
keymap_init();
bool handled = keymap_process_key(event.key);
```

**Only difference:** Pin definition syntax

**Pico:**
```c
const uint8_t row_pins[4] = {2, 3, 4, 5};
```

**STM32:**
```c
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOA, 0),
    MAKE_PIN(GPIOA, 1),
    MAKE_PIN(GPIOA, 4),
    MAKE_PIN(GPIOA, 5)
};
```

## Getting Started by Platform

### 🟢 Pico/Pico2 - Quick Start

1. ✅ Have Pico SDK installed
2. ✅ Copy root files (not `stm32/` folder)
3. ✅ Build with CMake
4. ✅ Flash `.uf2` file
5. ✅ Done!

**Time:** ~5 minutes

### 🟦 STM32 - Quick Start

1. ✅ Create project in STM32CubeMX
2. ✅ Enable USART2 for printf
3. ✅ Copy files from `stm32/` folder
4. ✅ Update pin definitions in main.c
5. ✅ Build and flash
6. ✅ Done!

**Time:** ~10-15 minutes

## File Mapping

| Purpose | Pico File | STM32 File |
|---------|-----------|------------|
| Driver header | `matrix.h` | `stm32/matrix_stm32.h` |
| Driver impl | `matrix.c` | `stm32/matrix_stm32.c` |
| Function header | `keymap_functions.h` | `stm32/keymap_functions_stm32.h` |
| Function impl | `keymap_functions.c` | `stm32/keymap_functions_stm32.c` |
| Example code | `main.c` | `stm32/main_example_*.c` |
| Build file | `CMakeLists.txt` | CubeMX project |
| Main README | `README.md` | `stm32/README_STM32.md` |

## Common Questions

### Q: Can I use the same code on both platforms?
**A:** The API is identical, but you need different driver files. Copy the logic from your main loop, but use the platform-specific matrix driver.

### Q: Which platform is easier?
**A:** Pico is simpler to set up. STM32 offers more flexibility and is better if you're already using STM32 in your project.

### Q: Can I use other pins on Pico?
**A:** Yes! Just change the pin numbers in `main.c`. Any GPIO works.

### Q: Which STM32 boards are supported?
**A:** Virtually all STM32 with HAL support: F0, F1, F3, F4, F7, G0, G4, H7, L0, L4, WB, WL.

### Q: Do I need external resistors?
**A:** No! Both drivers use internal pull-up resistors.

### Q: Can I add more keys (like 4x5)?
**A:** Yes, but requires modifying the driver. Change `MATRIX_ROWS` or `MATRIX_COLS` and update the pin arrays.

## Next Steps

### For Pico Users:
1. Read [README.md](README.md)
2. Build and test with PIN_TEST_MODE
3. Enable function mode (optional)
4. Read [FUNCTION_MODE.md](FUNCTION_MODE.md)

### For STM32 Users:
1. Read [stm32/README_STM32.md](stm32/README_STM32.md) ⭐
2. Pick your pins: [stm32/PIN_SELECTION_GUIDE.md](stm32/PIN_SELECTION_GUIDE.md)
3. Create CubeMX project
4. Copy files and build
5. Read [FUNCTION_MODE.md](FUNCTION_MODE.md) for advanced features

## Support

Both platforms share the same core logic, so most documentation applies to both. Check the platform-specific README for setup details.

