#ifndef MATRIX_KEYPAD_ROBUST_STM32_H
#define MATRIX_KEYPAD_ROBUST_STM32_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

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
#define DEBOUNCE_PRESS_MS   5    // 5ms debounce for press (faster response)
#define DEBOUNCE_RELEASE_MS 5    // 5ms debounce for release (faster response)
#define STUCK_KEY_TIMEOUT_MS 5000  // 5 seconds = stuck key

// Event queue configuration
#define EVENT_QUEUE_SIZE 32
#define ERROR_QUEUE_SIZE 8

// GPIO pin structure for STM32
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} GPIO_Pin_t;

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

// Statistics structure
typedef struct {
    uint32_t total_scans;
    uint32_t total_events;
    uint32_t total_errors;
    uint32_t queue_overflows;
    uint32_t max_scan_time_us;
    uint32_t avg_scan_time_us;
} ScanStatistics;

// Initialize the matrix keypad (robust version)
// Uses hardware timer (TIM2 by default) for scanning at precise intervals
// row_pins: array of 4 GPIO pins for rows (outputs)
// col_pins: array of 4 GPIO pins for columns (inputs with pull-ups + EXTI)
// htim: pointer to timer handle (e.g., &htim2)
// scan_frequency_hz: scanning frequency in Hz (default: 1000 = 1kHz)
void matrix_robust_init(const GPIO_Pin_t row_pins[4], const GPIO_Pin_t col_pins[4], 
                        TIM_HandleTypeDef *htim, uint32_t scan_frequency_hz);

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

// Enable EXTI interrupts for wake (columns trigger interrupt on key press)
void matrix_robust_enable_wake_interrupt(void);

// Disable EXTI wake interrupts
void matrix_robust_disable_wake_interrupt(void);

// Get statistics
void matrix_robust_get_statistics(ScanStatistics *stats);
void matrix_robust_reset_statistics(void);

// Power management
void matrix_robust_enter_low_power(void);  // Stop scanning, enable wake EXTI, enter STOP mode
void matrix_robust_exit_low_power(void);   // Resume scanning

// Timer interrupt callback - call this from HAL_TIM_PeriodElapsedCallback
void matrix_robust_timer_callback(TIM_HandleTypeDef *htim);

// EXTI interrupt callback - call this from HAL_GPIO_EXTI_Callback
void matrix_robust_exti_callback(uint16_t GPIO_Pin);

// Helper macros for pin definitions
#define MAKE_PIN(port, pin_num) {port, GPIO_PIN_##pin_num}

#endif // MATRIX_KEYPAD_ROBUST_STM32_H

