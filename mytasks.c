#include "mytasks.h"



volatile BaseType_t motion_detected = pdFALSE;
volatile BaseType_t sound_detected = pdFALSE;
QueueHandle_t alertQueue;


//=== Read PIR TASK ===
void vReadMotionSensorask(void *pvParameters){
	BaseType_t sensor_status; 
	for(;;){
		sensor_status = GPIO_PORTD_DATA_R & (1 << 2) ? pdTRUE : pdFALSE;
		if(sensor_status && !(motion_detected) && (!sound_detected)){
			motion_detected = pdTRUE;
			AlertType alert = ALERT_MOTION;
			BaseType_t queue_status = xQueueSendToBack(alertQueue, &alert, pdMS_TO_TICKS( 100 )); 
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

//=== Read Sound  TASK ===
void vReadSoundSensorask(void *pvParameters){
	BaseType_t sensor_status; 
	for(;;){
		sensor_status = GPIO_PORTD_DATA_R & (1 << 3) ? pdTRUE : pdFALSE;
		if(sensor_status && (!motion_detected) && (!sound_detected)){
			sound_detected = pdTRUE;
			AlertType alert = ALERT_SOUND;
			BaseType_t queue_status = xQueueSendToBack(alertQueue, &alert, pdMS_TO_TICKS( 100 )); 
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

//=== Alert Routine Task
void vAlertRoutineTask(void *pvParameters){
	AlertType rec_alert;
	for(;;){
		if (xQueueReceive(alertQueue, &rec_alert, portMAX_DELAY)){
			switch (rec_alert) {
					case ALERT_MOTION:
							UART3_OutString("Motion Detected \r\n");
							break;
					case ALERT_SOUND:
							UART3_OutString("Sound Detected  \r\n");
							break;
					default:
							break;
			}
			GPIO_PORTD_DATA_R |= (1<<1);
			GPIO_PORTF_DATA_R |= (1<<3);
			vTaskDelay(pdMS_TO_TICKS(5000));
			motion_detected = pdFALSE;
			sound_detected = pdFALSE;
		}
	}
}

void vHomeSafeTask(void *pvParameters) {
    while (1) {
        if ((!motion_detected) && (!sound_detected)) {
            UART3_OutString("Home Is Safe\r\n");
            GPIO_PORTF_DATA_R &= ~(1 << 3);  // Green LED OFF
            GPIO_PORTD_DATA_R &= ~(1 << 1);  // Buzzer OFF
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



