#ifndef MYTASKS_H
#define MYTASKS_H

#include "uart.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>

typedef enum {
    ALERT_NONE,
    ALERT_MOTION,
    ALERT_SOUND
} AlertType;

extern QueueHandle_t alertQueue;
extern SemaphoreHandle_t xBinarySemaphore;
extern volatile BaseType_t motion_detected;
extern volatile BaseType_t sound_detected;
extern volatile BaseType_t activation;



void UART3_Handler(void);
void UART3_INT_INIT(void);
void vReadMotionSensorask(void *pvParameters);
void vReadSoundSensorask(void *pvParameters);
void vAlertRoutineTask(void *pvParameters);
void vHomeSafeTask(void *pvParameters);
void vSystemActivationTask(void *pvParameters);
//void vSyncTask(void *pvParameters);


#endif
