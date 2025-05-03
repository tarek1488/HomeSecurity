#include "mytasks.h"

// === PortF Initialization for SW1 and Green LED ===
void PortF_Init(void) {
    SYSCTL_RCGCGPIO_R |= (1 << 5);       // Enable clock for Port F
    while ((SYSCTL_PRGPIO_R & (1 << 5)) == 0) {};  // Wait for Port F ready

    GPIO_PORTF_LOCK_R = 0x4C4F4344;       // Unlock PF0
    GPIO_PORTF_CR_R |= 0x1F;              // Allow changes to PF4-0
    GPIO_PORTF_DIR_R |= (1 << 3);         // PF3 (Green LED) output
    GPIO_PORTF_DIR_R &= ~(1 << 4);        // PF4 (SW1) input
    GPIO_PORTF_DEN_R |= 0x18;             // Enable digital on PF3, PF4
    GPIO_PORTF_PUR_R |= (1 << 4);         // Pull-up resistor on PF4
}

// === PORTD Init for motion and sound sensor
void portD_init(void){
    volatile unsigned long delay;
    SYSCTL_RCGCGPIO_R |= 0x08;        // Enable clock for Port D
    delay = SYSCTL_RCGCGPIO_R;        // Wait for clock to stabilize
    GPIO_PORTD_AMSEL_R = 0x00;        // Disable analog
    GPIO_PORTD_AFSEL_R = 0x00;        // Disable alternate functions
    GPIO_PORTD_DIR_R &= ~0x0C;        // PD2, PD3 as input
    GPIO_PORTD_DIR_R |= 0x03;         // PD0, PD1 as output (if needed)
    GPIO_PORTD_DEN_R |= 0x0F;         // Enable digital functions for PD0–PD3
}
//void vUARTTestTask(void *pvParameters) {
//    while (1) {
//			UART3_OutString("UART Working!\r\n");
//			vTaskDelay(1000 / portTICK_PERIOD_MS);
//        
//    }
//		
//}

int main(void) {
	
	UART3_Init();
	PortF_Init();
	portD_init();   // PD0–PD3 setup
	
	xSoundSemaphore = xSemaphoreCreateBinary();
	xMotionSemaphore = xSemaphoreCreateBinary();
//	xTaskCreate(vUARTTestTask, "UART Test", 1000, NULL, 1, NULL);
//	vTaskStartScheduler();
	
	if ((xSoundSemaphore != NULL) && (xMotionSemaphore != NULL)){
		

		//xTaskCreate(vSoundDetectedTask, "Sound Detected", 1000, NULL, 1, &SoundHandle);
		//xTaskCreate(vMotionDetectedTask, "Motion Detected", 1000, NULL, 1, &MotionHandle);
		// Initialize semaphores, interrupts, etc.
    xTaskCreate(vHomeSafeTask, "HomeSafeTask", 1000, NULL, 1, NULL);
    xTaskCreate(vMotionDetectedTask, "MotionTask", 1000, NULL, 2, NULL);
    xTaskCreate(vSoundDetectedTask, "SoundTask", 1000, NULL, 3, NULL);
		vTaskStartScheduler();
	}
	
	while (1) {}
}