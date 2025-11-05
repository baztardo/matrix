# STM32 Pin Selection Guide for Matrix Keypad

## Quick Reference

### Nucleo-F401RE Pin Suggestions

#### Option 1: Mixed Ports (Recommended)
```c
// Rows on Port A, Cols on Port B
Rows: PA0, PA1, PA4, PA5  (CN7 pins)
Cols: PB0, PB1, PB4, PB5  (CN10 pins)
```

#### Option 2: Arduino Headers
```c
// Using Arduino connector pins
Rows: PA0, PA1, PA4, PB0  (D0, D1, D2, D3)
Cols: PB5, PB4, PB10, PA8 (D4, D5, D6, D7)
```

### Nucleo-G070RB / G071RB Pin Suggestions

#### Option 1: All on Port C (Easiest Wiring)
```c
Rows: PC0, PC1, PC2, PC3
Cols: PC4, PC5, PC6, PC7
```

#### Option 2: Arduino Headers
```c
Rows: PA0, PA1, PA4, PB0  (D0-D3)
Cols: PC7, PB6, PA7, PA6  (D9, D10, D11, D12)
```

## Pin Selection Guidelines

### ✅ SAFE to Use

Most GPIO pins work fine. Prefer:
- Standard GPIO pins (not special function)
- 5V-tolerant pins (if your keypad runs at 5V)
- Pins on morpho/Arduino headers for easy access

### ⚠️ AVOID These Pins

**Never use:**
- **PA13, PA14** - SWD debugging pins (SWDIO, SWCLK)
- **PA11, PA12** - USB D-, D+ (if using USB)
- **PH0, PH1** - External oscillator (if used)

**Use with caution:**
- **PA2, PA3** - USART2 TX/RX (if using printf)
- **PA5** - May be connected to onboard LED (Nucleo boards)
- **Boot0** - Can affect boot mode
- Pins connected to onboard peripherals (check schematic)

### 🎯 Best Practices

1. **Group pins together** - Easier to route wires
2. **Use adjacent pins** - e.g., PA0-PA3 for rows
3. **Check 5V tolerance** - If keypad is 5V (most STM32 pins are FT)
4. **Leave debug pins free** - PA13/PA14 for SWD
5. **Document your choice** - Comment in code

## Example Configurations

### Configuration 1: Compact (Single Port)
```c
// All 8 pins on one port - cleanest wiring
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOC, 0),
    MAKE_PIN(GPIOC, 1),
    MAKE_PIN(GPIOC, 2),
    MAKE_PIN(GPIOC, 3)
};

const GPIO_Pin_t col_pins[4] = {
    MAKE_PIN(GPIOC, 4),
    MAKE_PIN(GPIOC, 5),
    MAKE_PIN(GPIOC, 6),
    MAKE_PIN(GPIOC, 7)
};
```

**Pros:** Clean, easy to wire, all on one port
**Cons:** Requires 8 free pins on same port

### Configuration 2: Arduino Shield Compatible
```c
// Using Arduino D0-D7 pins
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOA, 0),   // D0
    MAKE_PIN(GPIOA, 1),   // D1
    MAKE_PIN(GPIOA, 4),   // D2
    MAKE_PIN(GPIOB, 0)    // D3
};

const GPIO_Pin_t col_pins[4] = {
    MAKE_PIN(GPIOB, 5),   // D4
    MAKE_PIN(GPIOB, 4),   // D5
    MAKE_PIN(GPIOB, 10),  // D6
    MAKE_PIN(GPIOA, 8)    // D7
};
```

**Pros:** Standard header, compatible with shields
**Cons:** Pins scattered across ports (but still works fine)

### Configuration 3: Maximum Speed (Low Pin Numbers)
```c
// Lower numbered pins often have better routing
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOA, 0),
    MAKE_PIN(GPIOA, 1),
    MAKE_PIN(GPIOA, 2),  // Avoid if using USART2 TX
    MAKE_PIN(GPIOA, 3)   // Avoid if using USART2 RX
};

const GPIO_Pin_t col_pins[4] = {
    MAKE_PIN(GPIOB, 0),
    MAKE_PIN(GPIOB, 1),
    MAKE_PIN(GPIOB, 2),
    MAKE_PIN(GPIOB, 3)
};
```

**Pros:** Organized, sequential
**Cons:** May conflict with USART2 (PA2/PA3)

## Pin Mapping by Board

### Nucleo-F401RE

**Arduino Headers:**
| Arduino Pin | STM32 Pin | Safe for Keypad? |
|-------------|-----------|------------------|
| D0 | PA0 | ✅ Yes |
| D1 | PA1 | ✅ Yes |
| D2 | PA4 | ✅ Yes |
| D3 | PB0 | ✅ Yes |
| D4 | PB5 | ✅ Yes |
| D5 | PB4 | ✅ Yes |
| D6 | PB10 | ✅ Yes |
| D7 | PA8 | ✅ Yes |
| D8 | PA9 | ⚠️ Avoid (USB) |
| D13 | PA5 | ⚠️ LED |

**Morpho Connector (CN7/CN10):**
- Tons of GPIO available
- Check board user manual for details

### Nucleo-G070RB / G071RB

**Arduino Headers:**
| Arduino Pin | STM32 Pin | Safe for Keypad? |
|-------------|-----------|------------------|
| D0 | PC5 | ✅ Yes |
| D1 | PC4 | ✅ Yes |
| D2 | PA10 | ⚠️ Avoid (USB) |
| D3 | PB3 | ✅ Yes |
| D4 | PB5 | ✅ Yes |
| D5 | PB4 | ✅ Yes |
| D6 | PB10 | ✅ Yes |
| D7 | PA8 | ✅ Yes |

**Port C (Best Choice for G0):**
- PC0-PC7: All safe, all on one port

## Checking Your Pin Choice

### 1. Check Board Schematic
Download from st.com → User Manual → Schematic

Look for:
- Solder bridges (SB)
- Resistors to other components
- LEDs connected to pins

### 2. Check Pin Conflicts in CubeMX
- Open STM32CubeMX
- Assign your pins
- Look for yellow/red warnings

### 3. Test with Pin Test Mode
```c
#define PIN_TEST_MODE 1
```
Press keys and verify correct detection.

## 5V Tolerant Pins

Most STM32 GPIO pins are **5V tolerant (FT)** when configured as inputs.

**Check your datasheet** - look for "FT" or "FTf" in the pin table.

**Safe assumption:**
- Most GPIO = 5V tolerant ✅
- USB pins = NOT 5V tolerant ❌
- ADC-only pins = Usually not FT ❌

## Common Issues

### Issue: Some keys don't work
**Solution:** Check if pin is:
- Being used by another peripheral
- Has solder bridge connecting it elsewhere
- Connected to onboard component

### Issue: Keys work in test mode but not normal mode
**Solution:** 
- Increase debounce time
- Check for electrical noise
- Verify pull-ups are enabled

### Issue: CubeMX shows warning
**Solution:**
- Pick different pins
- Disable conflicting peripheral
- Check if pin has alternate function active

## Quick Copy-Paste Pin Definitions

### For F401RE (Tested Configuration)
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

### For G0 Series (Tested Configuration)
```c
const GPIO_Pin_t row_pins[4] = {
    MAKE_PIN(GPIOC, 0),   // PC0
    MAKE_PIN(GPIOC, 1),   // PC1
    MAKE_PIN(GPIOC, 2),   // PC2
    MAKE_PIN(GPIOC, 3)    // PC3
};

const GPIO_Pin_t col_pins[4] = {
    MAKE_PIN(GPIOC, 4),   // PC4
    MAKE_PIN(GPIOC, 5),   // PC5
    MAKE_PIN(GPIOC, 6),   // PC6
    MAKE_PIN(GPIOC, 7)    // PC7
};
```

## Need Help?

1. Check board schematic
2. Use STM32CubeMX pin assignment view
3. Try pin test mode
4. Check datasheet for pin features
5. Post your pin choices for review

Remember: **Almost any GPIO will work!** Just avoid the special pins listed above.

