#include "mytasks.h"

void PortF_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}

    GPIO_PORTF_LOCK_R = 0x4C4F4344;
    GPIO_PORTF_CR_R |= 0x1F;
    GPIO_PORTF_DIR_R |= (1 << 3);    // PF3 output
    GPIO_PORTF_DIR_R &= ~(1 << 4);   // PF4 input
    GPIO_PORTF_DEN_R |= 0x18;        // PF3, PF4 digital enable
    GPIO_PORTF_PUR_R |= (1 << 4);    // PF4 pull-up
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
		
	alertQueue = xQueueCreate(5, sizeof(AlertType)); // Queue of 5 alerts
    
	if (alertQueue != NULL) {
		xTaskCreate(vReadMotionSensorask, "Read Motion",128,NULL,3,NULL);
		xTaskCreate(vReadSoundSensorask, "Read Sound",128,NULL,3,NULL);
		xTaskCreate(vAlertRoutineTask, "Alert Manager", 128, NULL, 2, NULL);	
		xTaskCreate(vHomeSafeTask, "HomeSafe", 128, NULL, 1, NULL);
		vTaskStartScheduler();
	}

    while (1) {}
}
