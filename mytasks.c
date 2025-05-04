#include "mytasks.h"

volatile uint32_t motion_detected=0;
volatile uint32_t sound_detected=0;

SemaphoreHandle_t xMotionSemaphore;
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
						motion_detected = 1;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
						//GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_2);  // Re-enable
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
            sound_detected = 1;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
						//GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_3);  // Re-enable
						sound_detected = 0;
				}
        //vTaskDelay(200 / portTICK_PERIOD_MS); // reduce CPU usage
    }
}

// === Combined ISR for Port D ===
void GPIOD_Handler2(void) {
    UART3_OutString("DEBUG: Entered GPIOD_Handler\r\n");

    uint32_t status = GPIOD->MIS;  // Read masked interrupt status
    BaseType_t hpTaskWoken = pdFALSE;

    if (GPIOD->MIS & (1 << 2)) {  // PD2 (PIR)
        GPIOD->ICR = (1 << 2);  // Clear the interrupt
        UART3_OutString("DEBUG: Motion Detected\r\n");
				xSemaphoreGiveFromISR(xMotionSemaphore, &hpTaskWoken);
				UART3_OutString("DEBUG: seamphooooooooooor\r\n");
    }

    if (GPIOD->MIS & (1 << 3)) {  // PD3 (Sound)
        GPIOD->ICR = (1 << 3);  // Clear the interrupt
        UART3_OutString("DEBUG: Sound Detected\r\n");
        xSemaphoreGiveFromISR(xSoundSemaphore, &hpTaskWoken);
				
    }
		
    UART3_OutString("DEBUG: Exiting Handler\r\n");
		portYIELD_FROM_ISR(hpTaskWoken);
    
}


void SensorInterruptInit(void) {
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOIntRegister(GPIO_PORTD_BASE, GPIOD_Handler2);
    GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_RISING_EDGE);
    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
}
