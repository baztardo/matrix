#include <stdio.h>
#include "pico/stdlib.h"
#include "matrix_robust.h"
#include "keymap_functions.h"

// Example: Key event callback (called from ISR!)
void on_key_event(KeyEvent *event) {
    // Keep ISR handlers SHORT and FAST!
    // Don't use printf in ISR (can cause issues)
    // Just set a flag or add to your own queue
    
    // For this example, we'll let it queue and process in main loop
}

// Example: Error callback
void on_error(ErrorEvent *error) {
    // Handle errors - log, LED indication, etc.
    // This is called from ISR, keep it short!
}

int main() {
    // Initialize USB serial
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n=== ROBUST Matrix Keypad Driver ===\n");
    printf("Hardware timer + interrupts + error detection\n\n");
    
    // Define pin assignments
    const uint8_t row_pins[4] = {2, 3, 4, 5};
    const uint8_t col_pins[4] = {6, 7, 8, 9};
    
    // Initialize robust matrix keypad
    // Scan interval: 1000us = 1kHz = 1ms per scan
    matrix_robust_init(row_pins, col_pins, 1000);
    
    // Configure features
    matrix_robust_set_ghost_detection(true);      // Enable ghost key detection
    matrix_robust_set_stuck_detection(true, 5000); // 5 second stuck key timeout
    
    // Optional: Register callbacks (for ISR-driven events)
    // matrix_robust_set_key_callback(on_key_event);
    // matrix_robust_set_error_callback(on_error);
    
    // Initialize function mode
    keymap_init();
    
    // TEST: Simple timer to verify timer system works
    // Start scanning
    matrix_robust_start();
    sleep_ms(100);  // Allow timer to stabilize
    
    printf("\n✅ Keypad ready! Press keys...\n\n");
    
    KeyEvent event;
    ErrorEvent error;
    uint32_t last_stats_time = 0;
    uint32_t idle_count = 0;
    
    while (true) {
        // Process key events from queue
        while (matrix_robust_get_event(&event)) {
            idle_count = 0;  // Reset idle counter
            
            if (event.state == KEY_PRESSED) {
                // Process with function mode
                bool handled = keymap_process_key(event.key);
                
                if (!handled) {
                    printf("[%lu ms] Key: 0x%X (row=%d, col=%d)\n",
                           event.timestamp, event.key, event.row, event.col);
                }
            } else if (event.state == KEY_RELEASED) {
                printf("[%lu ms] Released: 0x%X\n", event.timestamp, event.key);
            }
        }
        
        // Process error events
        while (matrix_robust_get_error(&error)) {
            printf("⚠️  ERROR [%lu ms]: ", error.timestamp);
            switch (error.error_code) {
                case ERROR_STUCK_KEY:
                    printf("Stuck key detected (row=%d, col=%d)\n", 
                           error.row, error.col);
                    break;
                case ERROR_GHOST_KEY:
                    printf("Ghost key detected (row=%d, col=%d)\n", 
                           error.row, error.col);
                    break;
                default:
                    printf("Unknown error\n");
            }
        }
        
        // Print statistics every 60 seconds
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_stats_time > 60000) {
            ScanStatistics stats;
            matrix_robust_get_statistics(&stats);
            
            printf("\n--- Statistics ---\n");
            printf("Total scans:     %lu\n", stats.total_scans);
            printf("Total events:    %lu\n", stats.total_events);
            printf("Total errors:    %lu\n", stats.total_errors);
            printf("Queue overflows: %lu\n", stats.queue_overflows);
            printf("Max scan time:   %lu us\n", stats.max_scan_time_us);
            printf("Avg scan time:   %lu us\n", stats.avg_scan_time_us);
            printf("------------------\n\n");
            
            last_stats_time = now;
        }
        
        // Optional: Enter low power mode after 30 seconds of inactivity
        idle_count++;
        if (idle_count > 30000) {  // ~30 seconds (assuming 1ms sleep)
            printf("Entering low power mode...\n");
            matrix_robust_enter_low_power();
            
            // Sleep until key press wakes us
            __wfi();  // Wait for interrupt
            
            printf("Woke up from key press!\n");
            matrix_robust_exit_low_power();
            idle_count = 0;
        }
        
        // Small delay (not needed if using callbacks, but good for CPU)
        sleep_ms(1);
    }
    
    return 0;
}

