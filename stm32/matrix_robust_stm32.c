#include "matrix_robust_stm32.h"
#include <stdio.h>
#include <string.h>

// Default keymap
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

// Scanning state (accessed in ISR)
static volatile uint8_t current_row = 0;
static volatile uint8_t key_state[MATRIX_ROWS][MATRIX_COLS];
static volatile uint32_t key_timestamp[MATRIX_ROWS][MATRIX_COLS];
static volatile uint32_t debounce_time_press = DEBOUNCE_PRESS_MS;
static volatile uint32_t debounce_time_release = DEBOUNCE_RELEASE_MS;

// Event queues (circular buffers)
static KeyEvent event_queue[EVENT_QUEUE_SIZE];
static volatile uint32_t event_queue_head = 0;
static volatile uint32_t event_queue_tail = 0;

static ErrorEvent error_queue[ERROR_QUEUE_SIZE];
static volatile uint32_t error_queue_head = 0;
static volatile uint32_t error_queue_tail = 0;

// Callbacks
static volatile KeyEventCallback key_callback = NULL;
static volatile ErrorCallback error_callback = NULL;

// Timer handle
static TIM_HandleTypeDef *scan_timer = NULL;
static volatile bool scanning_active = false;

// Feature flags
static volatile bool ghost_detection_enabled = true;
static volatile bool stuck_detection_enabled = true;
static volatile uint32_t stuck_key_timeout = STUCK_KEY_TIMEOUT_MS;

// Statistics
static volatile ScanStatistics stats = {0};

// Ghost key detection buffer
static volatile uint8_t pressed_keys[MATRIX_ROWS][MATRIX_COLS];

// Forward declarations
static void scan_matrix(void);
static bool enqueue_event(KeyEvent *event);
static bool enqueue_error(ErrorEvent *error);
static bool detect_ghost_key(uint8_t row, uint8_t col);
static bool detect_stuck_key(uint8_t row, uint8_t col, uint32_t now);
static inline void delay_us(uint32_t us);

void matrix_robust_init(const GPIO_Pin_t row_pins[4], const GPIO_Pin_t col_pins[4],
                        TIM_HandleTypeDef *htim, uint32_t scan_frequency_hz) {
    // Copy pin assignments
    memcpy(row_gpios, row_pins, sizeof(GPIO_Pin_t) * MATRIX_ROWS);
    memcpy(col_gpios, col_pins, sizeof(GPIO_Pin_t) * MATRIX_COLS);
    
    // Copy default keymap
    memcpy(keymap, DEFAULT_KEYMAP, sizeof(keymap));
    
    // Store timer handle
    scan_timer = htim;
    
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
    
    // Configure timer for scanning
    // Timer should be configured in CubeMX to generate interrupt at scan_frequency_hz
    // For example: 1kHz = interrupt every 1ms
    
    // Clear state arrays
    memset((void*)key_state, KEY_IDLE, sizeof(key_state));
    memset((void*)key_timestamp, 0, sizeof(key_timestamp));
    memset((void*)pressed_keys, 0, sizeof(pressed_keys));
    memset((void*)&stats, 0, sizeof(stats));
    
    printf("Robust matrix keypad initialized (scan rate: %lu Hz)\n", scan_frequency_hz);
}

void matrix_robust_set_keymap(const uint8_t custom_keymap[4][4]) {
    memcpy(keymap, custom_keymap, sizeof(keymap));
}

void matrix_robust_start(void) {
    if (!scanning_active && scan_timer != NULL) {
        HAL_TIM_Base_Start_IT(scan_timer);
        scanning_active = true;
        printf("Scanning started\n");
    }
}

void matrix_robust_stop(void) {
    if (scanning_active && scan_timer != NULL) {
        HAL_TIM_Base_Stop_IT(scan_timer);
        scanning_active = false;
        printf("Scanning stopped\n");
    }
}

void matrix_robust_set_key_callback(KeyEventCallback callback) {
    key_callback = callback;
}

void matrix_robust_set_error_callback(ErrorCallback callback) {
    error_callback = callback;
}

// Timer interrupt callback - call from HAL_TIM_PeriodElapsedCallback
void matrix_robust_timer_callback(TIM_HandleTypeDef *htim) {
    if (htim == scan_timer) {
        scan_matrix();
    }
}

static void scan_matrix(void) {
    uint32_t scan_start = DWT->CYCCNT;
    stats.total_scans++;
    
    // Set all rows HIGH first
    for (int i = 0; i < MATRIX_ROWS; i++) {
        HAL_GPIO_WritePin(row_gpios[i].port, row_gpios[i].pin, GPIO_PIN_SET);
    }
    
    // Activate current row (set LOW)
    HAL_GPIO_WritePin(row_gpios[current_row].port, row_gpios[current_row].pin, GPIO_PIN_RESET);
    
    // Small delay for signal settling
    delay_us(1);
    
    uint32_t now = HAL_GetTick();
    
    // Read all columns for this row
    for (int col = 0; col < MATRIX_COLS; col++) {
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(col_gpios[col].port, col_gpios[col].pin);
        bool pressed = (pin_state == GPIO_PIN_RESET);  // LOW = pressed
        uint8_t current_state = key_state[current_row][col];
        uint32_t last_change = key_timestamp[current_row][col];
        
        // State machine with debouncing
        if (pressed) {
            if (current_state == KEY_IDLE) {
                // First detection of press
                key_timestamp[current_row][col] = now;
                key_state[current_row][col] = KEY_IDLE;  // Stay idle until debounced
            } else if (current_state == KEY_IDLE && (now - last_change) >= debounce_time_press) {
                // Debounced press confirmed
                
                // Ghost key detection
                if (ghost_detection_enabled && detect_ghost_key(current_row, col)) {
                    ErrorEvent error = {
                        .error_code = ERROR_GHOST_KEY,
                        .row = current_row,
                        .col = col,
                        .timestamp = now
                    };
                    enqueue_error(&error);
                    continue;
                }
                
                // Create press event
                KeyEvent event = {
                    .key = keymap[current_row][col],
                    .state = KEY_PRESSED,
                    .row = current_row,
                    .col = col,
                    .timestamp = now
                };
                
                key_state[current_row][col] = KEY_PRESSED;
                pressed_keys[current_row][col] = 1;
                
                // Enqueue or call callback
                if (key_callback) {
                    key_callback(&event);
                } else {
                    enqueue_event(&event);
                }
                
                stats.total_events++;
            } else if (current_state == KEY_PRESSED) {
                // Key is held
                key_state[current_row][col] = KEY_HELD;
                
                // Stuck key detection
                if (stuck_detection_enabled && detect_stuck_key(current_row, col, now)) {
                    ErrorEvent error = {
                        .error_code = ERROR_STUCK_KEY,
                        .row = current_row,
                        .col = col,
                        .timestamp = now
                    };
                    enqueue_error(&error);
                }
            }
        } else {
            // Key not pressed
            if (current_state == KEY_PRESSED || current_state == KEY_HELD) {
                // First detection of release
                key_timestamp[current_row][col] = now;
            } else if ((current_state == KEY_PRESSED || current_state == KEY_HELD) && 
                       (now - last_change) >= debounce_time_release) {
                // Debounced release confirmed
                KeyEvent event = {
                    .key = keymap[current_row][col],
                    .state = KEY_RELEASED,
                    .row = current_row,
                    .col = col,
                    .timestamp = now
                };
                
                key_state[current_row][col] = KEY_IDLE;
                pressed_keys[current_row][col] = 0;
                
                // Enqueue or call callback
                if (key_callback) {
                    key_callback(&event);
                } else {
                    enqueue_event(&event);
                }
                
                stats.total_events++;
            }
        }
    }
    
    // Deactivate current row
    HAL_GPIO_WritePin(row_gpios[current_row].port, row_gpios[current_row].pin, GPIO_PIN_SET);
    
    // Move to next row
    current_row = (current_row + 1) % MATRIX_ROWS;
    
    // Update scan time statistics
    uint32_t scan_time = (DWT->CYCCNT - scan_start) / (SystemCoreClock / 1000000);
    if (scan_time > stats.max_scan_time_us) {
        stats.max_scan_time_us = scan_time;
    }
    stats.avg_scan_time_us = ((stats.avg_scan_time_us * (stats.total_scans - 1)) + scan_time) / stats.total_scans;
}

static bool enqueue_event(KeyEvent *event) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    uint32_t next_head = (event_queue_head + 1) % EVENT_QUEUE_SIZE;
    
    if (next_head == event_queue_tail) {
        // Queue full
        stats.queue_overflows++;
        __set_PRIMASK(primask);
        return false;
    }
    
    event_queue[event_queue_head] = *event;
    event_queue_head = next_head;
    
    __set_PRIMASK(primask);
    return true;
}

static bool enqueue_error(ErrorEvent *error) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    uint32_t next_head = (error_queue_head + 1) % ERROR_QUEUE_SIZE;
    
    if (next_head == error_queue_tail) {
        __set_PRIMASK(primask);
        return false;
    }
    
    error_queue[error_queue_head] = *error;
    error_queue_head = next_head;
    stats.total_errors++;
    
    __set_PRIMASK(primask);
    
    if (error_callback) {
        error_callback(error);
    }
    
    return true;
}

bool matrix_robust_get_event(KeyEvent *event) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    if (event_queue_head == event_queue_tail) {
        __set_PRIMASK(primask);
        return false;  // Queue empty
    }
    
    *event = event_queue[event_queue_tail];
    event_queue_tail = (event_queue_tail + 1) % EVENT_QUEUE_SIZE;
    
    __set_PRIMASK(primask);
    return true;
}

bool matrix_robust_get_error(ErrorEvent *error) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    if (error_queue_head == error_queue_tail) {
        __set_PRIMASK(primask);
        return false;
    }
    
    *error = error_queue[error_queue_tail];
    error_queue_tail = (error_queue_tail + 1) % ERROR_QUEUE_SIZE;
    
    __set_PRIMASK(primask);
    return true;
}

bool matrix_robust_any_key_pressed(void) {
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            if (key_state[row][col] != KEY_IDLE) {
                return true;
            }
        }
    }
    return false;
}

uint32_t matrix_robust_get_event_count(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    uint32_t count;
    if (event_queue_head >= event_queue_tail) {
        count = event_queue_head - event_queue_tail;
    } else {
        count = EVENT_QUEUE_SIZE - event_queue_tail + event_queue_head;
    }
    
    __set_PRIMASK(primask);
    return count;
}

void matrix_robust_clear_events(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    event_queue_head = 0;
    event_queue_tail = 0;
    
    __set_PRIMASK(primask);
}

void matrix_robust_set_ghost_detection(bool enable) {
    ghost_detection_enabled = enable;
}

void matrix_robust_set_stuck_detection(bool enable, uint32_t timeout_ms) {
    stuck_detection_enabled = enable;
    stuck_key_timeout = timeout_ms;
}

static bool detect_ghost_key(uint8_t row, uint8_t col) {
    // Ghost keys occur when 3+ keys form a rectangle
    uint8_t keys_in_row = 0;
    uint8_t keys_in_col = 0;
    
    // Count pressed keys in this row
    for (int c = 0; c < MATRIX_COLS; c++) {
        if (pressed_keys[row][c]) keys_in_row++;
    }
    
    // Count pressed keys in this column
    for (int r = 0; r < MATRIX_ROWS; r++) {
        if (pressed_keys[r][col]) keys_in_col++;
    }
    
    // Simple ghost detection: if 2+ keys in row AND 2+ keys in col, might be ghost
    return (keys_in_row >= 2 && keys_in_col >= 2);
}

static bool detect_stuck_key(uint8_t row, uint8_t col, uint32_t now) {
    uint32_t press_time = key_timestamp[row][col];
    return ((now - press_time) > stuck_key_timeout);
}

void matrix_robust_enable_wake_interrupt(void) {
    // Reconfigure columns as EXTI inputs with falling edge trigger
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    for (int i = 0; i < MATRIX_COLS; i++) {
        GPIO_InitStruct.Pin = col_gpios[i].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(col_gpios[i].port, &GPIO_InitStruct);
    }
}

void matrix_robust_disable_wake_interrupt(void) {
    // Reconfigure columns back to normal inputs
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    for (int i = 0; i < MATRIX_COLS; i++) {
        GPIO_InitStruct.Pin = col_gpios[i].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(col_gpios[i].port, &GPIO_InitStruct);
    }
}

// EXTI interrupt callback - call from HAL_GPIO_EXTI_Callback
void matrix_robust_exti_callback(uint16_t GPIO_Pin) {
    // Check if it's one of our column pins
    for (int i = 0; i < MATRIX_COLS; i++) {
        if (GPIO_Pin == col_gpios[i].pin) {
            // Wake from low power - start scanning
            if (!scanning_active) {
                matrix_robust_exit_low_power();
            }
            break;
        }
    }
}

void matrix_robust_get_statistics(ScanStatistics *stats_out) {
    *stats_out = stats;
}

void matrix_robust_reset_statistics(void) {
    memset((void*)&stats, 0, sizeof(stats));
}

void matrix_robust_enter_low_power(void) {
    matrix_robust_stop();
    matrix_robust_enable_wake_interrupt();
    printf("Entered low power mode (wake on keypress)\n");
    
    // Optional: Enter STOP mode (uncomment if you want deep sleep)
    // HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}

void matrix_robust_exit_low_power(void) {
    // If using STOP mode, you may need to reconfigure clocks here
    // SystemClock_Config();
    
    matrix_robust_disable_wake_interrupt();
    matrix_robust_start();
    printf("Exited low power mode\n");
}

// Microsecond delay using DWT cycle counter
static inline void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < cycles);
}

