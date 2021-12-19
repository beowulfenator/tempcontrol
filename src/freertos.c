/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usbd_cdc_if.h"

#include <lcd_hd44780_i2c.h>

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

I2C_HandleTypeDef hi2c1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
void logString(char *data);
void StartDefaultTask(void *argument);
extern void MX_USB_DEVICE_Init(void);
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
  char logbuf[200] = {0};
  uint8_t addr = 0x27;
  uint8_t cbuf[6] = {0x00, 0x00, 0x12, 0xFF, 0x00, 0x00}; // I2C command buffer

  MX_USB_DEVICE_Init();

  logString("Started\n");
  if(HAL_I2C_IsDeviceReady(&hi2c1, (addr<<1), 3, 5) == HAL_OK) {
    sprintf(logbuf, "0x%2.2X SUCCESS\n", addr);
  } else {
    sprintf(logbuf, "0x%2.2X FAIL\n", addr);
  }
  logString(logbuf);

  lcdInit(&hi2c1, (uint8_t)0x26, (uint8_t)2, (uint8_t)20);

  for(;;)
  {
    cbuf[3] = ~cbuf[3];

    sprintf(logbuf, "I2C Transmit: %2.2X %2.2X to %2.2X\n", cbuf[0], cbuf[1], addr);
    logString(logbuf);

    if (HAL_I2C_Master_Transmit(&hi2c1, addr<<1, &cbuf[0], 2, HAL_MAX_DELAY) != HAL_OK) {
      logString("NOT HAL_OK\n");
    } else {
      logString("HAL_OK\n");
    }

    sprintf(logbuf, "I2C error code %" PRIu32 "\n", hi2c1.ErrorCode);
    logString(logbuf);

    sprintf(logbuf, "I2C Transmit: %2.2X %2.2X to %2.2X\n", cbuf[2], cbuf[3], addr);
    logString(logbuf);

    if (HAL_I2C_Master_Transmit(&hi2c1, addr<<1, &cbuf[2], 2, HAL_MAX_DELAY) != HAL_OK) {
      logString("NOT HAL_OK\n");
    } else {
      logString("HAL_OK\n");
    }

    sprintf(logbuf, "I2C error code %" PRIu32 "\n", hi2c1.ErrorCode);
    logString(logbuf);

    lcdSetCursorPosition(0, 0);
    lcdPrintStr((uint8_t*)"Time", 4);

    osDelay(1000);
  }
}

void logString(char *data) {
    while (CDC_Transmit_FS((uint8_t *)data, strlen(data)) != USBD_OK) {
      vTaskDelay(1);
    }
}
