#ifndef MATRIX_KEYPAD_ROBUST_H
#define MATRIX_KEYPAD_ROBUST_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

// Keypad configuration
#define MATRIX_ROWS 4
#define MATRIX_COLS 4

// Key states
#define KEY_IDLE       0
#define KEY_PRESSED    1
#define KEY_HELD       2
#define KEY_RELEASED   3

// Error states
#define ERROR_NONE          0
#define ERROR_STUCK_KEY     1
#define ERROR_GHOST_KEY     2
#define ERROR_SCAN_TIMEOUT  3

// Debounce settings (in milliseconds)
#define DEBOUNCE_PRESS_MS   20   // 20ms debounce for press
#define DEBOUNCE_RELEASE_MS 50   // 50ms debounce for release
#define STUCK_KEY_TIMEOUT_MS 5000  // 5 seconds = stuck key

// Scanning configuration
#define SCAN_INTERVAL_US 1000  // 1ms = 1kHz scan rate

// Key event structure
typedef struct {
    uint8_t key;         // Key value (0x0-0xF for hex keypad)
    uint8_t state;       // KEY_PRESSED, KEY_HELD, or KEY_RELEASED
    uint8_t row;         // Physical row (0-3)
    uint8_t col;         // Physical column (0-3)
    uint32_t timestamp;  // Timestamp in milliseconds
} KeyEvent;

// Error event structure
typedef struct {
    uint8_t error_code;
    uint8_t row;
    uint8_t col;
    uint32_t timestamp;
} ErrorEvent;

// Callback function types
typedef void (*KeyEventCallback)(KeyEvent *event);
typedef void (*ErrorCallback)(ErrorEvent *error);

// Initialize the matrix keypad (robust version)
// Uses hardware timer for scanning at precise intervals
// row_pins: array of 4 GPIO pins for rows (outputs)
// col_pins: array of 4 GPIO pins for columns (inputs with pull-ups + interrupts)
// scan_interval_us: scanning interval in microseconds (default: 1000 = 1kHz)
void matrix_robust_init(const uint8_t row_pins[4], const uint8_t col_pins[4], uint32_t scan_interval_us);

// Set custom key mapping
void matrix_robust_set_keymap(const uint8_t keymap[4][4]);

// Start scanning (enables timer interrupt)
void matrix_robust_start(void);

// Stop scanning (disables timer interrupt, saves power)
void matrix_robust_stop(void);

// Register callback for key events (called from ISR context!)
void matrix_robust_set_key_callback(KeyEventCallback callback);

// Register callback for error events
void matrix_robust_set_error_callback(ErrorCallback callback);

// Get next event from queue (thread-safe, non-blocking)
// Returns true if event was available
bool matrix_robust_get_event(KeyEvent *event);

// Get error event from queue
bool matrix_robust_get_error(ErrorEvent *error);

// Check if any key is currently pressed (thread-safe)
bool matrix_robust_any_key_pressed(void);

// Get number of events in queue
uint32_t matrix_robust_get_event_count(void);

// Clear all events from queue
void matrix_robust_clear_events(void);

// Enable/disable ghost key detection
void matrix_robust_set_ghost_detection(bool enable);

// Enable/disable stuck key detection
void matrix_robust_set_stuck_detection(bool enable, uint32_t timeout_ms);

// Enable interrupt-based wake (columns trigger interrupt on key press)
void matrix_robust_enable_wake_interrupt(void);

// Disable interrupt-based wake
void matrix_robust_disable_wake_interrupt(void);

// Get statistics
typedef struct {
    uint32_t total_scans;
    uint32_t total_events;
    uint32_t total_errors;
    uint32_t queue_overflows;
    uint32_t max_scan_time_us;
    uint32_t avg_scan_time_us;
} ScanStatistics;

void matrix_robust_get_statistics(ScanStatistics *stats);
void matrix_robust_reset_statistics(void);

// Power management
void matrix_robust_enter_low_power(void);  // Stop scanning, enable wake interrupt
void matrix_robust_exit_low_power(void);   // Resume scanning

#endif // MATRIX_KEYPAD_ROBUST_H

