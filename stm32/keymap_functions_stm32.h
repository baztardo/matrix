#ifndef KEYMAP_FUNCTIONS_STM32_H
#define KEYMAP_FUNCTIONS_STM32_H

#include <stdint.h>
#include <stdbool.h>

// Operating modes
typedef enum {
    MODE_NORMAL,      // Normal key entry mode
    MODE_FUNCTION     // Function/feature mode (triggered by F key)
} OperatingMode;

// Function pointer type for key actions
typedef void (*KeyActionFunc)(uint8_t key);

// Initialize the keymap function system
void keymap_init(void);

// Process a key press (handles mode switching and function dispatch)
// Returns true if key was handled (consumed), false if it should be passed through
bool keymap_process_key(uint8_t key);

// Get current operating mode
OperatingMode keymap_get_mode(void);

// Set a custom function for a specific key in function mode
// key: 0x0-0xE (F is reserved for mode toggle)
// func: function to call when that key is pressed in function mode
void keymap_set_function(uint8_t key, KeyActionFunc func);

// Clear a function mapping (key will do nothing in function mode)
void keymap_clear_function(uint8_t key);

// Example function implementations (you can customize these)
void function_key_1(uint8_t key);  // Example: Toggle LED
void function_key_2(uint8_t key);  // Example: Start timer
void function_key_3(uint8_t key);  // Example: Reset something
void function_key_4(uint8_t key);  // Example: Save settings
void function_key_5(uint8_t key);  // Example: Load settings

#endif // KEYMAP_FUNCTIONS_STM32_H

