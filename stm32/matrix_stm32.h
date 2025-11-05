#ifndef MATRIX_KEYPAD_STM32_H
#define MATRIX_KEYPAD_STM32_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"  // STM32 HAL includes

// Keypad configuration
#define MATRIX_ROWS 4
#define MATRIX_COLS 4

// Key states
#define KEY_IDLE       0
#define KEY_PRESSED    1
#define KEY_HELD       2
#define KEY_RELEASED   3

// Debounce settings (in scan cycles)
#define DEBOUNCE_PRESS   3   // Number of consistent reads to register press
#define DEBOUNCE_RELEASE 3   // Number of consistent reads to register release

// GPIO pin structure for STM32
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} GPIO_Pin_t;

// Key event structure
typedef struct {
    uint8_t key;      // Key value (0x0-0xF for hex keypad)
    uint8_t state;    // KEY_PRESSED, KEY_HELD, or KEY_RELEASED
    uint8_t row;      // Physical row (0-3)
    uint8_t col;      // Physical column (0-3)
} KeyEvent;

// Initialize the matrix keypad
// row_pins: array of 4 GPIO pins for rows (outputs)
// col_pins: array of 4 GPIO pins for columns (inputs with pull-ups)
void matrix_init(const GPIO_Pin_t row_pins[4], const GPIO_Pin_t col_pins[4]);

// Set custom key mapping (optional)
// keymap: 4x4 array defining what each position returns
// Default mapping is: 1,2,3,A / 4,5,6,B / 7,8,9,C / 0,F,E,D
void matrix_set_keymap(const uint8_t keymap[4][4]);

// Non-blocking scan function - call this frequently (e.g., in main loop or timer callback)
// Returns true if a key event occurred
bool matrix_scan(KeyEvent *event);

// Get current pressed key (0 if none, blocking check)
uint8_t matrix_get_key(void);

// Check if any key is currently pressed
bool matrix_any_key_pressed(void);

// Testing function - identifies pin mapping
// Call this in a loop and press keys to see row/col mapping
// Requires printf to be configured (e.g., UART redirect)
void matrix_test_pins(void);

// Helper macros for pin definitions (makes code more readable)
// Example: ROW_PIN(GPIOA, GPIO_PIN_0) or ROW_PIN(GPIOB, 1)
#define MAKE_PIN(port, pin_num) {port, GPIO_PIN_##pin_num}

#endif // MATRIX_KEYPAD_STM32_H

