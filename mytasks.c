#include "mytasks.h"



volatile BaseType_t motion_detected = pdFALSE;
volatile BaseType_t sound_detected = pdFALSE;
volatile BaseType_t activation = pdTRUE;
volatile BaseType_t homesent = pdFALSE;
QueueHandle_t alertQueue;
SemaphoreHandle_t xBinarySemaphore;


void UART3_Handler(void){
	UARTIntClear(UART3_BASE, UART_INT_RX | UART_INT_RT);
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	xSemaphoreGiveFromISR( xBinarySemaphore, &xHigherPriorityTaskWoken);
	
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void UART3_INT_INIT(void){	
	UARTIntDisable(UART3_BASE, UART_INT_RX | UART_INT_RT);
	IntDisable(INT_UART3);
	UARTIntRegister(UART3_BASE, UART3_Handler);  // Register ISR
	UARTIntClear(UART3_BASE, UART_INT_RX | UART_INT_RT);
	IntPrioritySet(INT_UART3, 0x80);
	UARTIntEnable(UART3_BASE, UART_INT_RX | UART_INT_RT);  // RX + timeout
	IntEnable(INT_UART3);// NVIC
}

//=== Read PIR TASK ===
void vReadMotionSensorask(void *pvParameters){
	BaseType_t sensor_status; 
	for(;;){
		sensor_status = GPIO_PORTD_DATA_R & (1 << 2) ? pdTRUE : pdFALSE;
		if(sensor_status && !(motion_detected) && (!sound_detected) && (activation)){
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
		if(sensor_status && (!motion_detected) && (!sound_detected) && (activation)){
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
			homesent = pdFALSE;
			vTaskDelay(pdMS_TO_TICKS(5000));
			motion_detected = pdFALSE;
			sound_detected = pdFALSE;
			GPIO_PORTF_DATA_R &= ~(1 << 3);  // Green LED OFF
      GPIO_PORTD_DATA_R &= ~(1 << 1);  // Buzzer OFF
		}
	}
}

void vHomeSafeTask(void *pvParameters) {
    while (1) {
        if(!homesent){
					if ((!motion_detected) && (!sound_detected) && (activation)) {
            UART3_OutString("Home Is Safe\r\n");
            GPIO_PORTF_DATA_R &= ~(1 << 3);  // Green LED OFF
            GPIO_PORTD_DATA_R &= ~(1 << 1);  // Buzzer OFF
						homesent = pdTRUE;
					}
				}
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vSystemActivationTask(void *pvParameters){
	char msg[32];
	int i = 0;
	for(;;){
		if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS){
			while (UARTCharsAvail(UART3_BASE)){
				char c = UARTCharGet(UART3_BASE);
				if (c == '\r' || c == '\n') continue;  // Skip CR/LF
				if (i < sizeof(msg) - 1) {
						msg[i++] = c;
				}
		  }
		msg[i] = '\0';  // Correct null-termination

		if (strcmp(msg, "ON") == 0) {
				UART3_OutString("Yes I received ON\r\n");
			}
		else{
			
		}
			UART3_OutString("Hello from else\r\n");
		}
		
	}
}





