#include "matrix_stm32.h"
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
static GPIO_Pin_t row_gpios[MATRIX_ROWS];
static GPIO_Pin_t col_gpios[MATRIX_COLS];

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

// Microsecond delay for STM32 (uses DWT cycle counter for precision)
static void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < cycles);
}

void matrix_init(const GPIO_Pin_t row_pins[4], const GPIO_Pin_t col_pins[4]) {
    // Copy pin assignments
    memcpy(row_gpios, row_pins, sizeof(GPIO_Pin_t) * MATRIX_ROWS);
    memcpy(col_gpios, col_pins, sizeof(GPIO_Pin_t) * MATRIX_COLS);
    
    // Copy default keymap
    memcpy(keymap, DEFAULT_KEYMAP, sizeof(keymap));
    
    // Enable DWT cycle counter for microsecond delays
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Initialize GPIO pins
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Rows: outputs, start HIGH (inactive)
    for (int i = 0; i < MATRIX_ROWS; i++) {
        GPIO_InitStruct.Pin = row_gpios[i].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(row_gpios[i].port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(row_gpios[i].port, row_gpios[i].pin, GPIO_PIN_SET);
    }
    
    // Columns: inputs with pull-up resistors
    for (int i = 0; i < MATRIX_COLS; i++) {
        GPIO_InitStruct.Pin = col_gpios[i].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(col_gpios[i].port, &GPIO_InitStruct);
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
        HAL_GPIO_WritePin(row_gpios[i].port, row_gpios[i].pin, GPIO_PIN_SET);
    }
    
    // Activate current row (set LOW)
    HAL_GPIO_WritePin(row_gpios[current_row].port, row_gpios[current_row].pin, GPIO_PIN_RESET);
    
    // Small delay for signal to settle (critical for fast scanning)
    delay_us(1);
    
    // Read all columns for this row
    for (int col = 0; col < MATRIX_COLS; col++) {
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(col_gpios[col].port, col_gpios[col].pin);
        bool pressed = (pin_state == GPIO_PIN_RESET);  // LOW = pressed (pulled down by keypress)
        
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
    HAL_GPIO_WritePin(row_gpios[current_row].port, row_gpios[current_row].pin, GPIO_PIN_SET);
    
    // Move to next row
    current_row = (current_row + 1) % MATRIX_ROWS;
    
    return event_detected;
}

uint8_t matrix_get_key(void) {
    // Scan all rows and return first pressed key
    for (int row = 0; row < MATRIX_ROWS; row++) {
        // Set all rows HIGH
        for (int i = 0; i < MATRIX_ROWS; i++) {
            HAL_GPIO_WritePin(row_gpios[i].port, row_gpios[i].pin, GPIO_PIN_SET);
        }
        
        // Activate this row
        HAL_GPIO_WritePin(row_gpios[row].port, row_gpios[row].pin, GPIO_PIN_RESET);
        delay_us(1);
        
        // Check columns
        for (int col = 0; col < MATRIX_COLS; col++) {
            if (HAL_GPIO_ReadPin(col_gpios[col].port, col_gpios[col].pin) == GPIO_PIN_RESET) {
                HAL_GPIO_WritePin(row_gpios[row].port, row_gpios[row].pin, GPIO_PIN_SET);
                return keymap[row][col];
            }
        }
        
        HAL_GPIO_WritePin(row_gpios[row].port, row_gpios[row].pin, GPIO_PIN_SET);
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
    printf("Row pins configured\n");
    printf("Col pins configured\n");
    printf("\nPress keys on the keypad...\n\n");
    
    while (true) {
        // Scan each row
        for (int row = 0; row < MATRIX_ROWS; row++) {
            // Set all rows HIGH
            for (int i = 0; i < MATRIX_ROWS; i++) {
                HAL_GPIO_WritePin(row_gpios[i].port, row_gpios[i].pin, GPIO_PIN_SET);
            }
            
            // Activate this row (LOW)
            HAL_GPIO_WritePin(row_gpios[row].port, row_gpios[row].pin, GPIO_PIN_RESET);
            HAL_Delay(1);
            
            // Read all columns
            for (int col = 0; col < MATRIX_COLS; col++) {
                if (HAL_GPIO_ReadPin(col_gpios[col].port, col_gpios[col].pin) == GPIO_PIN_RESET) {
                    printf("KEY DETECTED: Row index %d + Column index %d\n", row, col);
                    
                    // Wait for release
                    while (HAL_GPIO_ReadPin(col_gpios[col].port, col_gpios[col].pin) == GPIO_PIN_RESET) {
                        HAL_Delay(10);
                    }
                    HAL_Delay(200);  // Debounce delay
                }
            }
            
            // Deactivate row
            HAL_GPIO_WritePin(row_gpios[row].port, row_gpios[row].pin, GPIO_PIN_SET);
        }
        
        HAL_Delay(10);
    }
}

