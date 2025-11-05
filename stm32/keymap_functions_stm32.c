#include "keymap_functions_stm32.h"
#include <stdio.h>
#include <string.h>
#include "main.h"  // For GPIO access

#define MAX_FUNCTION_KEYS 15  // 0x0-0xE (F=0xF is reserved)

// Current operating mode
static OperatingMode current_mode = MODE_NORMAL;

// Function lookup table - maps keys to functions in function mode
static KeyActionFunc function_table[MAX_FUNCTION_KEYS];

void keymap_init(void) {
    // Clear all function mappings
    memset(function_table, 0, sizeof(function_table));
    
    // Set default function mappings (customize these!)
    function_table[0x1] = function_key_1;
    function_table[0x2] = function_key_2;
    function_table[0x3] = function_key_3;
    function_table[0x4] = function_key_4;
    function_table[0x5] = function_key_5;
    // Add more default mappings as needed...
    
    current_mode = MODE_NORMAL;
    printf("Keymap function system initialized.\n");
    printf("Press F to toggle function mode.\n");
}

bool keymap_process_key(uint8_t key) {
    // F key (0xF) toggles between normal and function mode
    if (key == 0xF) {
        if (current_mode == MODE_NORMAL) {
            current_mode = MODE_FUNCTION;
            printf("\n>>> FUNCTION MODE ACTIVATED <<<\n");
            printf("Press 1-E to trigger functions, F to exit.\n\n");
        } else {
            current_mode = MODE_NORMAL;
            printf("\n>>> NORMAL MODE <<<\n\n");
        }
        return true;  // Key consumed
    }
    
    // In function mode, execute mapped functions
    if (current_mode == MODE_FUNCTION) {
        if (key < MAX_FUNCTION_KEYS && function_table[key] != NULL) {
            printf("[Function Mode] Executing function for key 0x%X...\n", key);
            function_table[key](key);
            return true;  // Key consumed
        } else {
            printf("[Function Mode] No function mapped to key 0x%X\n", key);
            return true;  // Still consume the key
        }
    }
    
    // In normal mode, pass key through (return false = not consumed)
    return false;
}

OperatingMode keymap_get_mode(void) {
    return current_mode;
}

void keymap_set_function(uint8_t key, KeyActionFunc func) {
    if (key < MAX_FUNCTION_KEYS) {
        function_table[key] = func;
        printf("Function mapped to key 0x%X\n", key);
    }
}

void keymap_clear_function(uint8_t key) {
    if (key < MAX_FUNCTION_KEYS) {
        function_table[key] = NULL;
    }
}

// ============================================================================
// Example function implementations - CUSTOMIZE THESE FOR YOUR APPLICATION
// ============================================================================

void function_key_1(uint8_t key) {
    printf("  -> FUNCTION 1: Toggle LED\n");
    // Example for Nucleo boards (usually PA5 or LD2)
    // HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    // Or if you defined it manually:
    // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}

void function_key_2(uint8_t key) {
    printf("  -> FUNCTION 2: Start Timer / Counter\n");
    // Example: HAL_TIM_Base_Start(&htim2);
    // Add your code here
}

void function_key_3(uint8_t key) {
    printf("  -> FUNCTION 3: Reset / Clear Data\n");
    // Example: reset_system();
    // Add your code here
}

void function_key_4(uint8_t key) {
    printf("  -> FUNCTION 4: Save Settings to Flash\n");
    // Example: save_settings_to_flash();
    // Add your code here
}

void function_key_5(uint8_t key) {
    printf("  -> FUNCTION 5: Load Settings from Flash\n");
    // Example: load_settings_from_flash();
    // Add your code here
}

