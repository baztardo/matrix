/**
  ******************************************************************************
  * @file           : main_robust_example_g0.c
  * @brief          : Robust Matrix Keypad Example for STM32G0 series
  * @board          : STM32 Nucleo-G0 (G070RB, G071RB, etc.)
  * @features       : Hardware timer, event queue, error detection, power mgmt
  ******************************************************************************
  */

#include "main.h"
#include "matrix_robust_stm32.h"
#include "keymap_functions_stm32.h"
#include <stdio.h>

// Timer handle (configured in CubeMX)
extern TIM_HandleTypeDef htim3;  // Use TIM3 for scanning

// UART handle for printf
extern UART_HandleTypeDef huart2;

// Example pin configuration for Nucleo-G0
// All on Port C for easy wiring
// Rows:    PC0, PC1, PC2, PC3
// Columns: PC4, PC5, PC6, PC7

int main(void)
{
  // HAL Init
  HAL_Init();
  SystemClock_Config();
  
  // Initialize peripherals (done by CubeMX)
  // MX_GPIO_Init();
  // MX_USART2_UART_Init();
  // MX_TIM3_Init();  // Timer for scanning (1kHz)
  
  printf("\n\n=== ROBUST Matrix Keypad Driver for STM32 ===\n");
  printf("Board: Nucleo-G0 Series\n");
  printf("Features: Timer ISR + Queue + Error Detection + Power Mgmt\n\n");
  
  // Define pin assignments (all on Port C)
  const GPIO_Pin_t row_pins[4] = {
      MAKE_PIN(GPIOC, 0),   // Row 0 -> PC0
      MAKE_PIN(GPIOC, 1),   // Row 1 -> PC1
      MAKE_PIN(GPIOC, 2),   // Row 2 -> PC2
      MAKE_PIN(GPIOC, 3)    // Row 3 -> PC3
  };
  
  const GPIO_Pin_t col_pins[4] = {
      MAKE_PIN(GPIOC, 4),   // Col 0 -> PC4
      MAKE_PIN(GPIOC, 5),   // Col 1 -> PC5
      MAKE_PIN(GPIOC, 6),   // Col 2 -> PC6
      MAKE_PIN(GPIOC, 7)    // Col 3 -> PC7
  };
  
  // Initialize robust matrix keypad
  // TIM3 should be configured in CubeMX to generate 1kHz interrupt
  matrix_robust_init(row_pins, col_pins, &htim3, 1000);
  
  // Enable features
  matrix_robust_set_ghost_detection(true);
  matrix_robust_set_stuck_detection(true, 5000);  // 5 second timeout
  
  // Initialize function mode
  keymap_init();
  
  // Start scanning!
  matrix_robust_start();
  
  printf("Robust scanning active!\n");
  printf("Features enabled:\n");
  printf("  - Hardware timer (TIM3) at 1kHz\n");
  printf("  - Thread-safe event queue (32 events)\n");
  printf("  - Ghost key detection\n");
  printf("  - Stuck key detection (5s timeout)\n");
  printf("  - EXTI wake interrupts\n");
  printf("  - Low power support\n\n");
  
  KeyEvent event;
  ErrorEvent error;
  uint32_t last_stats_time = 0;
  uint32_t idle_count = 0;
  
  while (1)
  {
    // Process all queued events
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
    
    // Print statistics every 10 seconds
    uint32_t now = HAL_GetTick();
    if (now - last_stats_time > 10000) {
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
    if (idle_count > 30000) {  // ~30 seconds
      printf("Entering low power mode...\n");
      matrix_robust_enter_low_power();
      
      // Enter STOP mode (wake on EXTI) - G0 is very power efficient!
      // HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);
      // SystemClock_Config();  // Reconfigure clocks after wake
      
      // Or just sleep lightly
      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
      
      printf("Woke up from keypress!\n");
      idle_count = 0;
    }
    
    HAL_Delay(1);
  }
}

// Timer interrupt callback - must be called from stm32xxxx_it.c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  matrix_robust_timer_callback(htim);
}

// EXTI interrupt callback - for wake from low power
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  matrix_robust_exti_callback(GPIO_Pin);
}

// Printf redirection to UART
#ifdef __GNUC__
int _write(int fd, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}
#endif

