/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usbd_cdc_if.h"

#include <lcd_hd44780_i2c.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <sht3x.h>

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

I2C_HandleTypeDef hi2c1;
RTC_HandleTypeDef hrtc;
UART_HandleTypeDef huart1;


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
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;

  OneWire_HandleTypeDef ow;
  DallasTemperature_HandleTypeDef dt;
  CurrentDeviceAddress insideThermometer;

  // Create the handle for the sensor.
  sht3x_handle_t handle = {
    .i2c_handle = &hi2c1,
    .device_address = SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW
  };

  MX_USB_DEVICE_Init();

  logString("Started\n");
  if(HAL_I2C_IsDeviceReady(&hi2c1, (addr<<1), 3, 5) == HAL_OK) {
    sprintf(logbuf, "0x%2.2X SUCCESS\n", addr);
  } else {
    sprintf(logbuf, "0x%2.2X FAIL\n", addr);
  }
  logString(logbuf);

  lcdInit(&hi2c1, (uint8_t)0x26, (uint8_t)4, (uint8_t)20);

  for(;;){
    OW_Begin(&ow, &huart1);

    if(OW_Reset(&ow) == OW_OK) {
      logString("OneWire devices are present\n");
    }
    else {
      logString("OneWire no devices\n");
    }

    DT_SetOneWire(&dt, &ow);

    logString("Locating devices...\n");

    DT_Begin(&dt);

    uint8_t deviceCount = DT_GetDeviceCount(&dt);

    sprintf(logbuf, "Found %d devices\n", deviceCount);
    logString(logbuf);

    for (uint8_t m = 0; m < deviceCount; m++) {
      if (!DT_GetAddress(&dt, insideThermometer, m)) {
        logString("Unable to find address for Device\n");
      } else {
        sprintf(logbuf, "Device %d: ", m);
        logString(logbuf);
        for (uint8_t i = 0; i < 8; i++)
        {
          sprintf(logbuf, "0x%02X ", insideThermometer[i]);
          logString(logbuf);
        }
        logString("\n");
        sprintf(logbuf, "Device %d Resolution: %d\n", m, DT_GetResolution(&dt, insideThermometer));
        logString(logbuf);
      }
    }

    logString("Requesting temperatures...\n");
    DT_RequestTemperatures(&dt); // Send the command to get temperatures
    logString("DONE\n");

    for (uint8_t m = 0; m < deviceCount; m++) {
	    DT_GetAddress(&dt, insideThermometer, m);
      int16_t t = DT_GetTemp(&dt, insideThermometer);
      sprintf(logbuf, "Temperature for the device %d is: %d\n", m, t/128);
      logString(logbuf);

      sprintf(logbuf, "T%d: %d\xdf" "C", m, t/128);

      lcdSetCursorPosition(0, m);
      lcdPrintStr((uint8_t *)logbuf, strlen(logbuf));
    }

    // Initialise sensor (tests connection by reading the status register).
    if (!sht3x_init(&handle)) {
      logString("SHT3x access failed.\n");
    }

    // Read temperature and humidity.
    float temperature, humidity;
    sht3x_read_temperature_and_humidity(&handle, &temperature, &humidity);
    sprintf(logbuf, "%d\xdf" "C, %d%%RH", (int)temperature, (int)humidity);
    lcdSetCursorPosition(0, 2);
    lcdPrintStr((uint8_t *)logbuf, strlen(logbuf));

    osDelay(2000);
  }

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

    if(HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) == HAL_OK) {
      logString("Date HAL_OK\n");
    } else {
      logString("Date error\n");
    }

    if(HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) == HAL_OK) {
      logString("Time HAL_OK\n");
    } else {
      logString("Time error\n");
    }

    sprintf(logbuf, "20%2.2d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
    lcdSetCursorPosition(0, 1);
    lcdPrintStr((uint8_t *)logbuf, strlen(logbuf));

    logString("Printed date/time\n");

    osDelay(1000);
  }
}

void logString(char *data) {
    while (CDC_Transmit_FS((uint8_t *)data, strlen(data)) != USBD_OK) {
      vTaskDelay(1);
    }
}
