# Function Mode - User Guide

## Overview

Your keypad now has **two modes**:
- **Normal Mode**: Keys return their hex values (0x0-0xF)
- **Function Mode**: Keys 1-E trigger custom functions (F toggles modes)

## How to Use

### Entering Function Mode
1. Press **F** key
2. Display shows: `>>> FUNCTION MODE ACTIVATED <<<`
3. Keys 1-E now trigger functions instead of returning values

### Exiting Function Mode
1. Press **F** key again
2. Returns to normal mode

## Default Function Mappings

In Function Mode:
- **1** → Toggle LED / Output Pin
- **2** → Start Timer / Counter
- **3** → Reset / Clear Data
- **4** → Save Settings to Flash
- **5** → Load Settings from Flash
- **6-E** → Not mapped yet (you can add more)
- **F** → Exit function mode (reserved)

## Customizing Functions

### Method 1: Edit the Default Mappings

Edit `keymap_functions.c` and modify the function implementations:

```c
void function_key_1(uint8_t key) {
    printf("  -> FUNCTION 1: Your custom action\n");
    // Add your code here
    gpio_put(25, !gpio_get(25));  // Example: toggle onboard LED
}
```

### Method 2: Runtime Mapping

In your `main.c`, after `keymap_init()`:

```c
// Define your custom function
void my_custom_function(uint8_t key) {
    printf("Custom function executed!\n");
    // Your code here
}

// Map it to key 6
keymap_set_function(0x6, my_custom_function);
```

### Method 3: Add More Functions

In `keymap_functions.h`, add declaration:
```c
void function_key_6(uint8_t key);
```

In `keymap_functions.c`, implement it:
```c
void function_key_6(uint8_t key) {
    printf("  -> FUNCTION 6: My new feature\n");
    // Your code
}
```

Then in `keymap_init()`:
```c
function_table[0x6] = function_key_6;
```

## Example Use Cases

### Toggle an LED
```c
void function_key_1(uint8_t key) {
    static bool led_state = false;
    led_state = !led_state;
    gpio_put(LED_PIN, led_state);
    printf("LED %s\n", led_state ? "ON" : "OFF");
}
```

### Start/Stop a Timer
```c
void function_key_2(uint8_t key) {
    static bool timer_running = false;
    if (timer_running) {
        stop_timer();
        printf("Timer stopped\n");
    } else {
        start_timer();
        printf("Timer started\n");
    }
    timer_running = !timer_running;
}
```

### Change Display Mode
```c
void function_key_3(uint8_t key) {
    static uint8_t display_mode = 0;
    display_mode = (display_mode + 1) % 3;
    printf("Display mode: %d\n", display_mode);
    // Update display
}
```

## Architecture

### Flow Diagram
```
Key Press → matrix_scan() 
          → keymap_process_key()
          → If F pressed: toggle mode
          → If function mode: execute function_table[key]
          → If normal mode: return false (pass through)
```

### Files
- `keymap_functions.h` - API and function declarations
- `keymap_functions.c` - Mode logic and function implementations
- `main.c` - Integration with matrix scanner

### Function Pointer Table
The system uses a lookup table (`function_table[]`) that maps key values to function pointers. This is fast (O(1) lookup) and flexible.

## Tips

1. **Keep functions fast** - They run in the main loop
2. **Use static variables** - To maintain state between calls
3. **Provide feedback** - Print or show what happened
4. **F key is reserved** - Don't map it to a function
5. **Test thoroughly** - Especially if functions control hardware

## Troubleshooting

**Function mode not activating?**
- Make sure you're pressing F (bottom row, 2nd from right)
- Check serial output for mode messages

**Function not executing?**
- Verify it's mapped in `keymap_init()`
- Check for NULL pointer in function_table
- Look for printf output

**Want to disable function mode?**
- Comment out `keymap_init()` in main.c
- Remove the `keymap_process_key()` call
- Keys will always return normal values

## Advanced: Auto-Return to Normal Mode

To automatically exit function mode after a timeout, add this to `keymap_functions.c`:

```c
static uint32_t last_function_time = 0;
#define FUNCTION_TIMEOUT_MS 5000  // 5 seconds

bool keymap_process_key(uint8_t key) {
    // ... existing code ...
    
    if (current_mode == MODE_FUNCTION) {
        // Check timeout
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_function_time > FUNCTION_TIMEOUT_MS) {
            current_mode = MODE_NORMAL;
            printf(">>> AUTO-RETURN TO NORMAL MODE <<<\n");
        }
        last_function_time = now;
    }
    
    // ... rest of code ...
}
```

