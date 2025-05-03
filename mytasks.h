#include "uart.h"
#include <String.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <timers.h>
#include <task.h>


///Helper 
void delay_seconds(uint32_t seconds);

// === Motion Sensor


void MotionInterruptInit(void);
void MotionSensor_ISR_Handler(void);
void vMotionDetectedTask(void *pvParameters);



void SoundInterruptInit(void);
void SoundSensor_ISR_Handler(void);
void vSoundDetectedTask(void *pvParameters);
void vHomeSafeTask(void *pvParameters);

extern TaskHandle_t MotionHandle;
extern TaskHandle_t SoundHandle;
extern SemaphoreHandle_t xMotionSemaphore;
extern SemaphoreHandle_t xSoundSemaphore;
