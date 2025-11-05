#include <stdio.h>
#include "pico/stdlib.h"
#include "matrix.h"
#include "keymap_functions.h"

// Set to 1 to enable pin testing mode, 0 for normal operation
#define PIN_TEST_MODE 0

int main() {
    // Initialize USB serial
    stdio_init_all();
    
    // Wait for USB connection (optional, remove if not needed)
    sleep_ms(2000);
    
    printf("\n\n=== Matrix Keypad Driver for Pico2 ===\n");
    
    // Define pin assignments
    // Rows: GPIO 2, 3, 4, 5 (outputs)
    // Cols: GPIO 6, 7, 8, 9 (inputs with pull-ups)
    const uint8_t row_pins[4] = {2, 3, 4, 5};
    const uint8_t col_pins[4] = {6, 7, 8, 9};
    
    // Initialize the matrix keypad
    matrix_init(row_pins, col_pins);
    printf("Matrix keypad initialized!\n");
    printf("Rows: GPIO 2, 3, 4, 5\n");
    printf("Cols: GPIO 6, 7, 8, 9\n\n");
    
    // Initialize function mode system
    keymap_init();
    
#if PIN_TEST_MODE
    // Pin testing mode - helps you map your keypad
    printf("*** PIN TEST MODE ***\n");
    printf("Press keys to identify which GPIO maps to which physical position.\n");
    printf("Once you know the mapping, update the pin arrays and set PIN_TEST_MODE to 0.\n\n");
    
    matrix_test_pins();  // This runs forever
    
#else
    // Normal operation mode
    printf("*** NORMAL OPERATION MODE ***\n");
    printf("Non-blocking keypad scanning active.\n");
    printf("Press keys to see events.\n\n");
    
    KeyEvent event;
    uint32_t scan_count = 0;
    
    while (true) {
        // Non-blocking scan - call this as often as possible
        if (matrix_scan(&event)) {
            // Key event detected - only process PRESSED events
            if (event.state == KEY_PRESSED) {
                // Try to process as function mode key
                bool handled = keymap_process_key(event.key);
                
                if (!handled) {
                    // Normal mode - just display the key
                    printf("Key: 0x%X (row=%d, col=%d)\n", 
                           event.key, event.row, event.col);
                }
            }
            // Optionally show releases too (commented out to reduce clutter)
            // else if (event.state == KEY_RELEASED) {
            //     printf("Released: 0x%X\n", event.key);
            // }
        }
        
        scan_count++;
        
        // Optional: add a tiny delay to control scan rate
        // Remove this or reduce it for maximum speed
        sleep_us(100);  // 100us = 10kHz scan rate per row, 2.5kHz full scan
    }
#endif
    
    return 0;
}

