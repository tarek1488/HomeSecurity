
#include "mytasks.h"

//Helper 

void delay_seconds(uint32_t seconds) {
    // 1 second = 16,000,000 cycles at 16 MHz
    // SysCtlDelay takes 3 cycles per loop iteration
    SysCtlDelay(seconds * (16000000 / 3));
}


//=== Read Motion Task ===
TaskHandle_t MotionHandle; //Task Handle
SemaphoreHandle_t xMotionSemaphore;


void MotionSensor_ISR_Handler(void){
	//Clear flag
	GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_2);
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	xSemaphoreGiveFromISR(xMotionSemaphore, &xHigherPriorityTaskWoken);
	
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


void MotionInterruptInit(void){
	GPIOIntRegister(GPIO_PORTD_BASE,MotionSensor_ISR_Handler);
	GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_RISING_EDGE );
	GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_2);
	GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_2);
	IntMasterEnable();
}

//void vMotionDetectedTask(void *pvParameters){
//	
//	while(1){
//		if(xSemaphoreTake(xMotionSemaphore, pdMS_TO_TICKS(250)) == pdTRUE){
//			UART3_OutString("Motion Detected \r\n");
//			GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
//			GPIO_PORTD_DATA_R |= (1 << 1); // set PD1 to HIGH for Buzzer
//			vTaskDelay(5000 / portTICK_PERIOD_MS);
//		}
//		else {
//			UART3_OutString("Home Is Safe\r\n");  
//			GPIO_PORTF_DATA_R &= ~(1 << 3);
//			GPIO_PORTD_DATA_R &= ~(1 << 1);
//    }
//    vTaskDelay(200 / portTICK_PERIOD_MS); // reduce CPU usage
//		
//	}
//}




//=== Read Sound Task ===
TaskHandle_t SoundHandle;
SemaphoreHandle_t xSoundSemaphore;
void SoundSensor_ISR_Handler(void){
	//Clear flag
	GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_3);
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	xSemaphoreGiveFromISR(xSoundSemaphore, &xHigherPriorityTaskWoken);
	
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


void SoundInterruptInit(void){
	GPIOIntRegister(GPIO_PORTD_BASE,SoundSensor_ISR_Handler);
	GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_3, GPIO_RISING_EDGE );
	GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_3);
	GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_3);
	IntMasterEnable();
}

//void vSoundDetectedTask(void *pvParameters){
//	while(1){
//		if(xSemaphoreTake(xSoundSemaphore, pdMS_TO_TICKS(250)) == pdTRUE){
//			UART3_OutString("Sound Detected \r\n");
//			GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
//			GPIO_PORTD_DATA_R |= (1 << 1); // set PD1 to HIGH for Buzzer
//			vTaskDelay(5000 / portTICK_PERIOD_MS);
//		} 
//		else {
//				UART3_OutString("Home Is Safe\r\n");
//        GPIO_PORTF_DATA_R &= ~(1 << 3);
//        GPIO_PORTD_DATA_R &= ~(1 << 1);
//    }
//    vTaskDelay(200 / portTICK_PERIOD_MS); // reduce CPU usage
//		
//	}
//}

// Home Safe Task (Sends "Home is Safe" every 200ms)
void vHomeSafeTask(void *pvParameters) {
    while(1) {
        UART3_OutString("Home Is Safe\r\n");
        GPIO_PORTF_DATA_R &= ~(1 << 3);  // Turn off the Green LED
        GPIO_PORTD_DATA_R &= ~(1 << 1);  // Turn off the Buzzer
        
        vTaskDelay(200 / portTICK_PERIOD_MS);  // Send "Home is Safe" every 200 ms
    }
}

// Motion Task
void vMotionDetectedTask(void *pvParameters) {
    while(1) {
        if(xSemaphoreTake(xMotionSemaphore, portMAX_DELAY == pdTRUE)) {
            UART3_OutString("Motion Detected \r\n");
            GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
            GPIO_PORTD_DATA_R |= (1 << 1); // set PD1 to HIGH for Buzzer
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        } else {
            // Home Safe will handle turning off LED and Buzzer periodically
        }
        vTaskDelay(200 / portTICK_PERIOD_MS); // reduce CPU usage
    }
}

// Sound Task
void vSoundDetectedTask(void *pvParameters) {
    while(1) {
        if(xSemaphoreTake(xSoundSemaphore, portMAX_DELAY) == pdTRUE) {
            UART3_OutString("Sound Detected \r\n");
            GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
            GPIO_PORTD_DATA_R |= (1 << 1); // set PD1 to HIGH for Buzzer
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        } else {
            // Home Safe will handle turning off LED and Buzzer periodically
        }
        vTaskDelay(200 / portTICK_PERIOD_MS); // reduce CPU usage
    }
}




