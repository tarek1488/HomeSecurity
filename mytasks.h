#include "uart.h"
#include <String.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <timers.h>
#include <task.h>

extern SemaphoreHandle_t xMotionSemaphore;
extern SemaphoreHandle_t xSoundSemaphore;
extern volatile uint32_t motion_detected;
extern volatile uint32_t sound_detected;

void vMotionDetectedTask(void *pvParameters);
void vSoundDetectedTask(void *pvParameters);
void vHomeSafeTask(void *pvParameters);

void GPIOD_Handler2(void);
void SensorInterruptInit(void);

