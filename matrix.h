#ifndef MATRIX_KEYPAD_H
#define MATRIX_KEYPAD_H

#include <stdint.h>
#include <stdbool.h>

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
void matrix_init(const uint8_t row_pins[4], const uint8_t col_pins[4]);

// Set custom key mapping (optional)
// keymap: 4x4 array defining what each position returns
// Default mapping is: 1,2,3,A / 4,5,6,B / 7,8,9,C / E,0,F,D
void matrix_set_keymap(const uint8_t keymap[4][4]);

// Non-blocking scan function - call this frequently (e.g., in main loop)
// Returns true if a key event occurred
bool matrix_scan(KeyEvent *event);

// Get current pressed key (0 if none, blocking check)
uint8_t matrix_get_key(void);

// Check if any key is currently pressed
bool matrix_any_key_pressed(void);

// Testing function - identifies pin mapping
// Call this in a loop and press keys to see row/col mapping
void matrix_test_pins(void);

#endif // MATRIX_KEYPAD_H

