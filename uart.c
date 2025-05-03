#include "uart.h"


void UART3_Init(void) {
    SYSCTL_RCGCUART_R |= (1 << 3);     // Enable UART3 clock
    SYSCTL_RCGCGPIO_R |= (1 << 2);      // Enable GPIOC clock
    while ((SYSCTL_PRGPIO_R & (1 << 2)) == 0) {};  // Wait for Port C ready

    GPIO_PORTC_AFSEL_R |= 0xC0;         // PC6, PC7 alternate functions
    GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & 0x00FFFFFF) | (0x11 << 24); // PC7=TX, PC6=RX
    GPIO_PORTC_DEN_R |= 0xC0;            // Enable digital on PC6, PC7
    GPIO_PORTC_AMSEL_R &= ~0xC0;          // Disable analog on PC6, PC7

    UART3_CTL_R &= ~(1 << 0);             // Disable UART
    UART3_IBRD_R = 104;                   // Integer Baud rate 9600
    UART3_FBRD_R = 11;                    // Fractional Baud rate
    UART3_LCRH_R = (0x3 << 5);             // 8-bit word length
    UART3_CTL_R |= (1 << 0) | (1 << 8) | (1 << 9); // Enable UART, TXE, RXE
		UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet(), 9600,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

// === Send a single character through UART3 ===
void UART3_OutChar(char data) {
    while ((UART3_FR_R & (1 << 5)) != 0); // Wait until TXFF is 0 (Transmit FIFO Full flag clear)
    UART3_DR_R = data;
}

// === Send a full string through UART3 ===
void UART3_OutString(const char* str) {
    while (*str) {
        UART3_OutChar(*str++);
    }
}




char UART3_Receiver(void)  
{
    char data;
	  while((UART3->FR & (1<<4)) != 0); /* wait until Rx buffer is not full */
    data = UART3->DR ;  	/* before giving it another byte */
    return (unsigned char) data;
}



