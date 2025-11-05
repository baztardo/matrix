#include "matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Default keymap for 4x4 hex keypad
static const uint8_t DEFAULT_KEYMAP[4][4] = {
    {0x1, 0x2, 0x3, 0xA},
    {0x4, 0x5, 0x6, 0xB},
    {0x7, 0x8, 0x9, 0xC},
    {0x0, 0xF, 0xE, 0xD}
};

// Pin configuration
static uint8_t row_gpios[MATRIX_ROWS];
static uint8_t col_gpios[MATRIX_COLS];

// Current keymap
static uint8_t keymap[MATRIX_ROWS][MATRIX_COLS];

// Scanning state
static uint8_t current_row = 0;
static uint8_t last_key_state[MATRIX_ROWS][MATRIX_COLS];
static uint8_t debounce_counter[MATRIX_ROWS][MATRIX_COLS];
static uint8_t current_key_state[MATRIX_ROWS][MATRIX_COLS];

// Last detected key for tracking
static uint8_t last_pressed_row = 0xFF;
static uint8_t last_pressed_col = 0xFF;

void matrix_init(const uint8_t row_pins[4], const uint8_t col_pins[4]) {
    // Copy pin assignments
    memcpy(row_gpios, row_pins, MATRIX_ROWS);
    memcpy(col_gpios, col_pins, MATRIX_COLS);
    
    // Copy default keymap
    memcpy(keymap, DEFAULT_KEYMAP, sizeof(keymap));
    
    // Initialize GPIO pins
    // Rows: outputs, start HIGH (inactive)
    for (int i = 0; i < MATRIX_ROWS; i++) {
        gpio_init(row_gpios[i]);
        gpio_set_dir(row_gpios[i], GPIO_OUT);
        gpio_put(row_gpios[i], 1);  // High = inactive
    }
    
    // Columns: inputs with pull-up resistors
    for (int i = 0; i < MATRIX_COLS; i++) {
        gpio_init(col_gpios[i]);
        gpio_set_dir(col_gpios[i], GPIO_IN);
        gpio_pull_up(col_gpios[i]);  // Pull-up to avoid floating
    }
    
    // Clear state arrays
    memset(last_key_state, KEY_IDLE, sizeof(last_key_state));
    memset(current_key_state, KEY_IDLE, sizeof(current_key_state));
    memset(debounce_counter, 0, sizeof(debounce_counter));
}

void matrix_set_keymap(const uint8_t custom_keymap[4][4]) {
    memcpy(keymap, custom_keymap, sizeof(keymap));
}

bool matrix_scan(KeyEvent *event) {
    bool event_detected = false;
    
    // Set all rows HIGH first
    for (int i = 0; i < MATRIX_ROWS; i++) {
        gpio_put(row_gpios[i], 1);
    }
    
    // Activate current row (set LOW)
    gpio_put(row_gpios[current_row], 0);
    
    // Small delay for signal to settle (critical for fast scanning)
    sleep_us(1);
    
    // Read all columns for this row
    for (int col = 0; col < MATRIX_COLS; col++) {
        bool pressed = !gpio_get(col_gpios[col]);  // LOW = pressed (pulled down by keypress)
        
        // Debouncing logic
        if (pressed) {
            if (debounce_counter[current_row][col] < DEBOUNCE_PRESS) {
                debounce_counter[current_row][col]++;
            }
        } else {
            if (debounce_counter[current_row][col] > 0) {
                debounce_counter[current_row][col]--;
            }
        }
        
        // Determine current state based on debounce counter
        uint8_t new_state = KEY_IDLE;
        if (debounce_counter[current_row][col] >= DEBOUNCE_PRESS) {
            new_state = KEY_PRESSED;
        }
        
        // Detect state changes
        if (new_state != last_key_state[current_row][col]) {
            if (new_state == KEY_PRESSED && last_key_state[current_row][col] == KEY_IDLE) {
                // Key press detected
                if (event && !event_detected) {
                    event->key = keymap[current_row][col];
                    event->state = KEY_PRESSED;
                    event->row = current_row;
                    event->col = col;
                    event_detected = true;
                    last_pressed_row = current_row;
                    last_pressed_col = col;
                }
                current_key_state[current_row][col] = KEY_PRESSED;
            } else if (new_state == KEY_IDLE && last_key_state[current_row][col] == KEY_PRESSED) {
                // Key release detected
                if (event && !event_detected) {
                    event->key = keymap[current_row][col];
                    event->state = KEY_RELEASED;
                    event->row = current_row;
                    event->col = col;
                    event_detected = true;
                }
                current_key_state[current_row][col] = KEY_IDLE;
            }
            last_key_state[current_row][col] = new_state;
        } else if (new_state == KEY_PRESSED && last_key_state[current_row][col] == KEY_PRESSED) {
            // Key held
            current_key_state[current_row][col] = KEY_HELD;
        }
    }
    
    // Deactivate current row
    gpio_put(row_gpios[current_row], 1);
    
    // Move to next row
    current_row = (current_row + 1) % MATRIX_ROWS;
    
    return event_detected;
}

uint8_t matrix_get_key(void) {
    // Scan all rows and return first pressed key
    for (int row = 0; row < MATRIX_ROWS; row++) {
        // Set all rows HIGH
        for (int i = 0; i < MATRIX_ROWS; i++) {
            gpio_put(row_gpios[i], 1);
        }
        
        // Activate this row
        gpio_put(row_gpios[row], 0);
        sleep_us(1);
        
        // Check columns
        for (int col = 0; col < MATRIX_COLS; col++) {
            if (!gpio_get(col_gpios[col])) {
                gpio_put(row_gpios[row], 1);
                return keymap[row][col];
            }
        }
        
        gpio_put(row_gpios[row], 1);
    }
    
    return 0;  // No key pressed
}

bool matrix_any_key_pressed(void) {
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            if (current_key_state[row][col] != KEY_IDLE) {
                return true;
            }
        }
    }
    return false;
}

void matrix_test_pins(void) {
    printf("\n=== Matrix Keypad Pin Tester ===\n");
    printf("Row pins: GPIO %d, %d, %d, %d\n", 
           row_gpios[0], row_gpios[1], row_gpios[2], row_gpios[3]);
    printf("Col pins: GPIO %d, %d, %d, %d\n", 
           col_gpios[0], col_gpios[1], col_gpios[2], col_gpios[3]);
    printf("\nPress keys on the keypad...\n\n");
    
    while (true) {
        // Scan each row
        for (int row = 0; row < MATRIX_ROWS; row++) {
            // Set all rows HIGH
            for (int i = 0; i < MATRIX_ROWS; i++) {
                gpio_put(row_gpios[i], 1);
            }
            
            // Activate this row (LOW)
            gpio_put(row_gpios[row], 0);
            sleep_us(10);
            
            // Read all columns
            for (int col = 0; col < MATRIX_COLS; col++) {
                if (!gpio_get(col_gpios[col])) {
                    printf("KEY DETECTED: Row GPIO %d (index %d) + Column GPIO %d (index %d)\n",
                           row_gpios[row], row, col_gpios[col], col);
                    
                    // Wait for release
                    while (!gpio_get(col_gpios[col])) {
                        sleep_ms(10);
                    }
                    sleep_ms(200);  // Debounce delay
                }
            }
            
            // Deactivate row
            gpio_put(row_gpios[row], 1);
        }
        
        sleep_ms(10);
    }
}

