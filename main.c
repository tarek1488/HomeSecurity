#include "mytasks.h"
#include "lcd_code.h"
void PortF_Init(void) {
    SYSCTL_RCGCGPIO_R |= 0x20;
    unsigned long delay = SYSCTL_RCGCGPIO_R;
		GPIO_PORTF_DEN_R = 0x0E;
		GPIO_PORTF_DIR_R = 0x0E;
}

void PortD_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)) {}

    GPIO_PORTD_AMSEL_R = 0x00;
    GPIO_PORTD_AFSEL_R = 0x00;
    GPIO_PORTD_DIR_R &= ~0x0C;       // PD2, PD3 input
    GPIO_PORTD_DIR_R |= 0x03;        // PD0, PD1 output (e.g., buzzer)
    GPIO_PORTD_DEN_R |= 0x0F;
}

int main(void) {
	
	UART3_Init();
	PortF_Init();
	PortD_Init();
	I2C0_Init();
	LCD_Init();
	alertQueue = xQueueCreate(5, sizeof(AlertType)); // Queue of 5 alerts
  xBinarySemaphore = xSemaphoreCreateBinary(); 
	LCD_SetCursor(0,0);
	if ((alertQueue != NULL) && xBinarySemaphore !=NULL) {
		LCD_Clear();
		xTaskCreate(vSystemActivationTask,"systm activation",128,NULL,4,NULL);
		xTaskCreate(vReadMotionSensorask, "Read Motion",128,NULL,3,NULL);
		xTaskCreate(vReadSoundSensorask, "Read Sound",128,NULL,3,NULL);
		xTaskCreate(vAlertRoutineTask, "Alert Manager", 128, NULL, 2, NULL);	
		xTaskCreate(vHomeSafeTask, "HomeSafe", 128, NULL, 1, NULL);
		UART3_INT_INIT();
		vTaskStartScheduler();
	}

    while (1) {}
}
