#include "uart.h"
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
// === Delay Function 
void delay_seconds(uint32_t seconds) {
    // 1 second = 16,000,000 cycles at 16 MHz
    // SysCtlDelay takes 3 cycles per loop iteration
    SysCtlDelay(seconds * (16000000 / 3));
}




// === Alert Variables
uint32_t home_alert;
uint32_t sound_alert; // port d pin 3
uint32_t motion_alert; // port d pin 2

int main(void) {
	
	UART3_Init();
	PortF_Init();
	portD_init();   // PD0–PD3 setup
	
	

	while (1) {
		
		
		//Reading sound sensor
		sound_alert = (GPIO_PORTD_DATA_R & (1<<3)) ? 1 : 0;
		
		//Reading motion sensor
		motion_alert = (GPIO_PORTD_DATA_R & (1<<2)) ? 1 : 0;
		
		//Both alert happens
		home_alert = sound_alert & motion_alert;
		//sound_alert = 1;
		
		//Reading activation value from esp
		char activation = UART3_Receiver();
		GPIO_PORTF_DATA_R = 0x02;
		
		if (activation == 'A'){
			GPIO_PORTF_DATA_R = 0x04;
			if(sound_alert){
				UART3_OutString("Sound Detected  \r\n");
				GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
				GPIO_PORTD_DATA_R |= (1 << 1);
				delay_seconds(4);
			}
			else if(motion_alert){
				UART3_OutString("Motion Detected \r\n");
				GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
				GPIO_PORTD_DATA_R |= (1 << 1);
				delay_seconds(4);
				
			}
			else if(home_alert){
				UART3_OutString("Home Unsecure   \r\n");
				GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
				delay_seconds(4);
			}
			else{
				UART3_OutString("Home is Safe    \r\n");
				GPIO_PORTF_DATA_R &= ~(1 << 3);          // Turn OFF Green LED
				GPIO_PORTD_DATA_R &= ~(1 << 1);
				delay_seconds(4);
			}
			delay_seconds(1);
		}
		else if (activation == 'B'){
				UART3_OutString("System Off   \r\n");
				GPIO_PORTF_DATA_R |= (1 << 3); // Turn ON Green LED
				delay_seconds(2);
		}
		else{
			GPIO_PORTF_DATA_R == 0x0C; // Turn ON Green LED
			//UART3_OutString("JUNK");
			UART3_OutString(activation);
			delay_seconds(4);
		}	
	           
    }
}





















