
#include "mytasks.h"

//Helper 
volatile uint32_t motion_detected=0;
volatile uint32_t sound_detected=0;
volatile uint8_t scheduler_started = 0;     // Define and initialize here




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
	
}




//=== Read Sound Task ===
TaskHandle_t SoundHandle;
SemaphoreHandle_t xSoundSemaphore;

// Home Safe Task (Sends "Home is Safe" every 200ms)
void vHomeSafeTask(void *pvParameters) {
    while(1) {
      if ((!motion_detected) && (!sound_detected)){   					
				UART3_OutString("Home Is Safe\r\n");
				GPIO_PORTF_DATA_R &= ~(1 << 3);  // Turn off the Green LED
				GPIO_PORTD_DATA_R &= ~(1 << 1);  // Turn off the Buzzer				
			}
			vTaskDelay(2000 / portTICK_PERIOD_MS);  // Send "Home is Safe" every 2s
    }
}

// Motion Task
void vMotionDetectedTask(void *pvParameters) {
    while(1) {
        if(xSemaphoreTake(xMotionSemaphore, portMAX_DELAY) == pdTRUE) {
            UART3_OutString("Motion Detected \r\n");
            GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
            GPIO_PORTD_DATA_R |= (1 << 1); // set PD1 to HIGH for Buzzer
						//motion_detected = 1;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
						GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_2);  // Re-enable
						motion_detected = 0;
        }
        //vTaskDelay(500 / portTICK_PERIOD_MS); // reduce CPU usage
    }
}

// Sound Task
void vSoundDetectedTask(void *pvParameters) {
    while(1) {
				UART3_OutString("Sound Detected \r\n");
        if(xSemaphoreTake(xSoundSemaphore, portMAX_DELAY) == pdTRUE) {
            
						UART3_OutString("Sound Detected \r\n");
            GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
            GPIO_PORTD_DATA_R |= (1 << 1); // set PD1 to HIGH for Buzzer
            //sound_detected = 1;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
						GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_3);  // Re-enable
						sound_detected = 0;
				}
        //vTaskDelay(200 / portTICK_PERIOD_MS); // reduce CPU usage
    }
}

//////////// other approach 
// === Combined ISR for Port D ===
void GPIODzeby_Handler(void) {
    UART3_OutString("DEBUG: Entered GPIOD_Handler\r\n");

    uint32_t status = GPIOIntStatus(GPIO_PORTD_BASE, true);
    BaseType_t hpTaskWoken = pdFALSE;

    if (status & GPIO_PIN_2) {
        GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_2);
				GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_2);
        UART3_OutString("DEBUG: if 1\r\n");
        xSemaphoreGiveFromISR(xMotionSemaphore, &hpTaskWoken);
    }

    if (status & GPIO_PIN_3) {
        GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_3);  // Disable temporarily
        GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_3);
        UART3_OutString("DEBUG: if 2\r\n");
        xSemaphoreGiveFromISR(xSoundSemaphore, &hpTaskWoken);
    }

    UART3_OutString("DEBUG: Out from Handler\r\n");
    portYIELD_FROM_ISR(hpTaskWoken);
}



void SensorInterruptInit(void) {
    // Configure PD2 and PD3 for rising edge interrupts
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOD ->IS &= ~(GPIO_PIN_2 | GPIO_PIN_3);  // Edge-sensitive
    GPIOD ->IBE  &= ~(GPIO_PIN_2 | GPIO_PIN_3); // Not both edges
    GPIOD->IEV |=  (GPIO_PIN_2 | GPIO_PIN_3); // Rising edge

    GPIOIntRegister(GPIO_PORTD_BASE, GPIODzeby_Handler);
    GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    NVIC_SetPriority(GPIOD_IRQn, 3);
    NVIC_EnableIRQ(GPIOD_IRQn);
}







