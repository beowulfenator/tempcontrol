/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include <lcd_hd44780_i2c.h>

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
void StartDefaultTask(void *argument);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

void MX_FREERTOS_Init(void) {
  /* add mutexes, ... */
  /* add semaphores, ... */
  /* start timers, add new ones, ... */
  /* add queues, ... */
  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  /* add threads, ... */
  /* add events, ... */
}

void StartDefaultTask(void *argument)
{
    I2C_HandleTypeDef hi2c1;

    lcdInit(&hi2c1, (uint8_t)0x27, (uint8_t)2, (uint8_t)20);
    
    // Print text and home position 0,0
    lcdPrintStr((uint8_t*)"Hello,", 6);
    
    // Set cursor at zero position of line 3
    lcdSetCursorPosition(0, 2);

    // Print text at cursor position
    lcdPrintStr((uint8_t*)"World!", 6);

  for(;;)
  {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    osDelay(500);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    osDelay(100);
  }
}
