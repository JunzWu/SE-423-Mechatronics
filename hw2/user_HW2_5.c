/******************************************************************************
MSP430G2553 Project Creator

GE 423  - Dan Block
        Spring(2017)

        Written(by) : Steve(Keres)
College of Engineering Control Systems Lab
University of Illinois at Urbana-Champaign
*******************************************************************************/

#include "msp430g2553.h"
#include "UART.h"

char newprint = 0;
int timecnt = 0;  
int timecheck = 0;  

char received_val = 0;

void main(void) {

	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

	if (CALBC1_16MHZ ==0xFF || CALDCO_16MHZ == 0xFF) while(1);

	DCOCTL = CALDCO_16MHZ;    // Set uC to run at approximately 16 Mhz
	BCSCTL1 = CALBC1_16MHZ; 

	// Initialize Port 1
	P1SEL &= ~0xF0;  // Set P1.4 - P1.7 as GPIO
	P1SEL2 &= ~0xF0; //
	P1REN = 0x00;    // No resistors enabled for Port 1
	P1DIR |= 0xF0;   // Set P1.4 - P1.7 to output to drive LEDs
	P1OUT &= ~0xF0;  // Initially set P1.4 - P1.7 to 0


	// Timer A Config
	TACCTL0 = CCIE;       		// Enable Periodic interrupt
	TACCR0 = 16000;                // period = 1ms   
	TACTL = TASSEL_2 + MC_1; // source SMCLK, up mode


	Init_UART(9600,1);	// Initialize UART for 9600 baud serial communication

	_BIS_SR(GIE); 		// Enable global interrupt


	while(1) {

		if(newmsg) {

			newmsg = 0;
		}

		if (newprint)  {

			UART_printf("%d\n\r",received_val); // divide by two so displays seconds by default

			newprint = 0;
		}

	}
}


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	timecheck++; // Keep track of time for main while loop. 

	if (timecheck == 1000) {
	    timecheck = 0;
	    timecnt++;
	    newprint = 1;  // flag main while loop that 1 second have gone by.
	}

}


/*
// ADC 10 ISR - Called when a sequence of conversions (A7-A0) have completed
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {

}
*/


// USCI Transmit ISR - Called when TXBUF is empty (ready to accept another character)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void) {

	if(IFG2&UCA0TXIFG) {		// USCI_A0 requested TX interrupt
		if(printf_flag) {
			if (currentindex == txcount) {
				senddone = 1;
				printf_flag = 0;
				IFG2 &= ~UCA0TXIFG;
			} else {
				UCA0TXBUF = printbuff[currentindex];
				currentindex++;
			}
		} else if(UART_flag) {
			if(!donesending) {
				UCA0TXBUF = txbuff[txindex];
				if(txbuff[txindex] == 255) {
					donesending = 1;
					txindex = 0;
				}
				else txindex++;
			}
		} else {  // interrupt after sendchar call so just set senddone flag since only one char is sent
			senddone = 1;
		}

		IFG2 &= ~UCA0TXIFG;
	}

	if(IFG2&UCB0TXIFG) {	// USCI_B0 requested TX interrupt (UCB0TXBUF is empty)

		IFG2 &= ~UCB0TXIFG;   // clear IFG
	}
}


// USCI Receive ISR - Called when shift register has been transferred to RXBUF
// Indicates completion of TX/RX operation
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {

	if(IFG2&UCB0RXIFG) {  // USCI_B0 requested RX interrupt (UCB0RXBUF is full)

		IFG2 &= ~UCB0RXIFG;   // clear IFG, for USCIB
	}

	if(IFG2&UCA0RXIFG) {  // USCI_A0 requested RX interrupt (UCA0RXBUF is full)

		// Uncomment this block of code if you would like to use this COM protocol that uses 253 as STARTCHAR and 255 as STOPCHAR

		// The received ASCII value of a character is placed in the register UCA0RXBUF
		// global char received_val

		received_val = UCA0RXBUF;


		// ASCII character '1' => 49 => turn on LED 1
		// ASCII character '2' => 50 => turn on LED 2
		// ASCII character '3' => 51 => turn on LED 3
		// ASCII character '4' => 52 => turn on LED 4

		// ASCII character '5' => 53 => turn off LED 1
		// ASCII character '6' => 54 => turn off LED 2
		// ASCII character '7' => 55 => turn off LED 3
		// ASCII character '8' => 56 => turn off LED 4

		if (received_val == '1') {
			// Turn on LED 1
			P1OUT |= 0x10;
		}
		else if (received_val == '2'){
			// Turn on LED 2
			P1OUT |= 0x20;
		}
		else if (received_val == '3'){
			// Turn on LED 3
			P1OUT |= 0x40;
		}
		else if (received_val == '4'){
			// Turn on LED 4
			P1OUT |= 0x80;
		}
		else if (received_val == '5'){
			// Turn off LED 1
			P1OUT &= ~0x10;
		}
		else if (received_val == '6'){
			// Turn off LED 2
			P1OUT &= ~0x20;
		}
		else if (received_val == '7'){
			// Turn off LED 3
			P1OUT &= ~0x40;
		}
		else if (received_val == '8'){
			// Turn off LED 4
			P1OUT &= ~0x80;
		}
		else
		{
			// Turn off all LED 1 - LED 4
			P1OUT &= ~0xF0;
		}

		sendchar(received_val);

		/*
 	    if(!started) {	// Haven't started a message yet
			if(UCA0RXBUF == 253) {
				started = 1;
				newmsg = 0;
			}
		}
		else {	// In process of receiving a message		
			if((UCA0RXBUF != 255) && (msgindex < (MAX_NUM_FLOATS*5))) {
				rxbuff[msgindex] = UCA0RXBUF;

				msgindex++;
			} else {	// Stop char received or too much data received
				if(UCA0RXBUF == 255) {	// Message completed
					newmsg = 1;
					rxbuff[msgindex] = 255;	// "Null"-terminate the array
				}
				started = 0;
				msgindex = 0;
			}
		}
		*/

		// All the code for HW2 Exercise 5
		// The USCIA is the only serial port on the MSP430G2553 that can be configured as a RS-232 UART
		IFG2 &= ~UCA0RXIFG; //For USCIA


	}

}


