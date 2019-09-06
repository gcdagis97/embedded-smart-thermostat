// Authors: George Dagis (CE), NAME OMITTED (CE), NAME OMITTED (CE)
// Professor: Michael Otis
// EGC 443 Embedded Systems
// 3-30-18
// v1.0

/* This program acts as a basic thermostat heating / cooling system. Meant to be used with a Tiva™ TM4C123GH6PM
Microcontroller
	
	Program converts analog output from an LM35Dz temperature sensor and convert it to temperature in Celsius
	A desired temperature is established and is to be maintained by the system
	If external temperature exceeds the desired temperature by 1-2 degrees, an AC system indicated by a blue LED is turned on
	If external temperature is less than the set temperature by 1-2 degrees, a heating system indicated by a red LED is turned on
	Using interrupts via a timer, the ADC is sampled once every second
	Both the external and desired temperature are refreshed and displayed using terminal software such as TeraTerm or PuTTY

*/

#include "TM4C123GH6PM.h"
#include <stdio.h>

//Program utilizes UART functionality
void UART0Tx(char c);
void UART0_init(void);
void UART0_puts(char* s);

//Program utilizes timer
void Timer1_init(void);

//Program utilizes delay
void delayMs(int n);

//Integer variable temperature. This is the temperature that is read from the LM35Dz temperature sensor
int currtemp;

//Desired temperature
int destemp = 70;

//Buffer
char buffer[16];

//Main method
int main(void) {

	//To Run Timer
	Timer1_init();

	// Initialize UART0 for output
	UART0_init();

	// Enable Clocks for GPIOE and ADC0
	SYSCTL->RCGCGPIO |= 0x10; // Enable clock to GPIOE
	SYSCTL->RCGCADC |= 1; // Enable clock to ADC0

	//Initialize PE3 for AIN0 input
	GPIOE->AFSEL |= 8; // Enable alternate function
	GPIOE->DEN &= ~8; // Disable digital function
	GPIOE->AMSEL |= 8; // Enable analog function
	
	// Initialize ADC0
	ADC0->ACTSS &= ~8; // Disable SS3 during configuration
	ADC0->EMUX &= ~0xF000; // Software trigger conversion
	ADC0->SSMUX3 = 0; // Get input from channel 0
	ADC0->SSCTL3 |= 6; // Take one sample at a time, set flag at 1st sample
	ADC0->ACTSS |= 8; // Enable ADC0 sequencer 3
	
	//GPIOA and Configurations
	SYSCTL->RCGCGPIO |= 0x03; // Enable clock for Port A and Port B
	SYSCTL->RCGCGPIO |= 0x20; // Enable clock for Port F
	GPIOA->DIR &= ~0x30; // PORTA5 switch
	GPIOA->PUR |= 0x30; // Enable pull up for PORTF4, 0
	GPIOA->DEN |= 0x30; // PORTA5 dig
	
	//GPIOB Configurations
	GPIOB->DIR |= 0x20; // PORTB5 Blue LED
	GPIOB->DIR |= 0x40; // PORTB6 Red LED
	GPIOB->DEN |= 0x20; // PORTB5 dig
	GPIOB->DEN |= 0x40; // PORTB6 dig
	
	//GPIOF Configurations
	GPIOF->AFSEL = 0x0E; /* PF1, PF2, PF3 uses alternate function */
	GPIOF->PCTL &= ~0x0000FFF0; /* make PF1, PF2, PF3 PWM output pin */
	GPIOF->PCTL |= 0x00005550;
	GPIOF->DIR |= 0x0E; // Enable GPIOF Pins 3,2,1
	GPIOF->DEN |= 0x0E; // Enable dig
	GPIOA->IS &= ~0x30; // Make bit 6 edge sensitive
	GPIOA->IBE &= ~0x30; // Trigger is controlled by IEV
	GPIOA->IEV &= ~0x30; // Falling edge trigger
	GPIOA->ICR |= 0x30; // Clear any prior interrupt
	GPIOA->IM |= 0x30; // Unmask interrupt
	
	//Test
	GPIOF->DATA = 0x0E;
	SYSCTL->RCGCPWM |= 2; /* enable clock to PWM1 */
	SYSCTL->RCGCGPIO |= 0x20; /* enable clock to PORTF */
	SYSCTL->RCC &= ~0x00100000; /* no pre-divide for PWM clock */
	
	//PWM config.
	//Enables generator 2 and pin F1
	PWM1->_2_CTL = 0x0; /* stop counter */
	PWM1->_2_GENB = 0x0000008C; /* M1PWM7 output set when reload, */
	
	/* clear when match PWMCMPA */
	PWM1->_2_LOAD = 16000; /* set load value for 1kHz (16MHz/16000) */
	PWM1->_2_CMPA = 10000; /* set duty cycle to min */
	PWM1->_2_CTL = 1;
	
	//enables generator 3, Pins F2, F3
	PWM1->_3_CTL = 0x0; /* stop counter */
	PWM1->_3_GENB = 0x0000008C; /* M1PWM7 output set when reload, */
	
	/* clear when match PWMCMPA */
	PWM1->_3_GENA = 0x0000080C;
	PWM1->_3_LOAD =16000; /* set load value for 1kHz (16MHz/16000) */
	PWM1->_3_CMPA = 8000; /* set duty cycle to min */
	PWM1->_3_CTL = 1; /* start timer */
	PWM1->_3_CMPB = 1000; /* set duty cycle to min */
	PWM1->ENABLE = 0xE0; /* start PWM Channels 5,6,7 */
	
	//Enable interrupt in NVIC and set priority to 3. Enable IRQ's
	NVIC->IP[0] = 3 << 5; // set interrupt priority to 3
	NVIC->ISER[0] |= 0x1; // enable IRQ0 (D0 of ISER[0])
	__enable_irq(); // global enable IRQs
	while(1){}
}

//UART0
void UART0_init(void) {

	//Provide & enable clocks
	SYSCTL->RCGCUART |= 1; // Provide clock to UART0
	SYSCTL->RCGCGPIO |= 0x01; // Enable clock to GPIOA
	
	// UART0 initialization
	UART0->CTL = 0; // Disable UART0
	UART0->IBRD = 104; // 16MHz/16=1MHz, 1MHz/104=9600 baud rate
	UART0->FBRD = 11; // Fraction part, see Example 4-4
	UART0->CC = 0; // Use system clock
	UART0->LCRH = 0x60; // 8-bit, no parity, 1-stop bit, no FIFO
	UART0->CTL = 0x301; // Enable UART0, TXE, RXE
	
	// UART0 TX0 and RX0 use PA0 and PA1. Set them up.
	GPIOA->DEN = 0x03; // Make PA0 and PA1 as digital
	GPIOA->AFSEL = 0x03; // Use PA0,PA1 alternate function
	GPIOA->PCTL = 0x11; // Configure PA0 and PA1 for UART
}

//Timer method for interrupts
//Method takes input from LM35Dz and prints external temperature as well as set temperature.
//Method also controls red and blue LED logic, or "AC" and "Heat"
void TIMER1A_Handler(void) {

	int difference = destemp-currtemp;
	int p;
	int xc, xe, xh;
	ADC0->PSSI |= 8; // Start a conversion sequence 3
	
	//Get temperature from LM35Dz, print external temperature and desired temperature. Wait 1 second and repeat.
	while((ADC0->RIS & 8) == 0) ; // wait for conversion complete
	currtemp = (ADC0->SSFIFO3 * 594/4096) + 27;
	ADC0->ISC = 8; // clear completion flag
	sprintf(buffer, "\r\nCurrent Temperature = %dF, Desired Temperature = %dF\n", currtemp, destemp);
	UART0_puts(buffer);
	delayMs(1000);

	//////////////////////////////////////////////////////////////////////////////
	/*
	PF1 Heating fan -> PWM1->_2_CMPA
	PF2 Cooling fan -> PWM1->_3_CMPB
	PF3 Exhaust fan -> PWM1->_3_CMPA
	*/

	if(difference<=2.5) {
		p=1-(.133*(difference-2.5));
		xc = p*15999;
		xh = 15000;
		PWM1->_2_CMPA = xc; //turn on cool
		PWM1->_3_CMPB = xh;
		GPIOB->DATA |= 0x020;
		GPIOB->DATA &= ~0x040; //turn off red LED
	}

	else if(-7.5>=difference>=7.5) {
		p=1-(.133*(difference-7.5));
		xe = p*15999;
		PWM1->_3_CMPA = xe;
	}

	else {
		p=(.133*(difference-2.5));
		xh = p*15999;
		PWM1->_3_CMPB = xh;
		
		//Turn on blue LED
		GPIOB->DATA |= 0x040; //Turn on red LED
		GPIOB->DATA &= ~0x020; //turn off blue LED
	}
}

void Timer1_init(void) {
	SYSCTL->RCGCTIMER |= 2; // Enable clock to Timer Module 1
	TIMER1->CTL = 0; // Disable Timer1 before initialization
	TIMER1->CFG = 0x04; // 16-bit option
	TIMER1->TAMR = 0x02; // Periodic mode and up-counter
	TIMER1->TAPR = 250; // 16000000 Hz / 250 = 64000 Hz
	TIMER1->TAILR = 64000; // 64000 Hz / 64000 = 1 Hz
	TIMER1->ICR = 0x1; // Clear the Timer1A timeout flag
	TIMER1->IMR |= 0x01; // Enable Timer1A timeout interrupt
	TIMER1->CTL |= 0x01; // Enable Timer1A after initialization
	NVIC->IP[21] = 4 << 5; // Set interrupt priority to 4
	NVIC->ISER[0] |= 0x00200000; // Enable IRQ21 (D21 of ISER[0])
}

/*
SW1 is connected to PA4 pin, SW2 is connected to PA5. Both of them trigger PORTF interrupt
If SW1 is pressed, decrease desired temperature by 1 degree C
If SW2 is pressed, increase desired temperature by 1 degree C
A delay of 1/4 second was added to avoid settemp incrementing or decrementing more than 1 time in 1 button press.
*/

//Switch Handler
void GPIOA_Handler(void) {
	volatile int readback;

	// If SW1 (PA4) is pressed
	if (GPIOA->MIS & 0x10) {
		//Decrement settemp by 2
		destemp = destemp - 2;
		GPIOA->ICR |= 0x30; // Clear the interrupt flag
		readback = GPIOA->ICR; // A read to force clearing of interrupt flag
		delayMs(250);
	}
	
	// Else if SW2 (PA5) is pressed
	else if (GPIOA->MIS & 0x20) {
		//Increment settemp by 2
		destemp = destemp + 2;
		GPIOA->ICR |= 0x30; // Clear the interrupt flag
		readback = GPIOA->ICR; // A read to force clearing of interrupt flag
		delayMs(250);
	}
	
	// We shouldn't ever get here
	else {
		GPIOA->ICR = GPIOA->MIS;
		readback = GPIOA->ICR; // A read to force clearing of interrupt flag
	}
}

void UART0Tx(char c) {
	while((UART0->FR & 0x20) != 0); // Wait until Tx buffer not full
		UART0->DR = c; // Before giving it another byte
}

void UART0_puts(char* s) {
	while (*s != 0) // If not end of string
		UART0Tx(*s++); // Send the character through UART0
}

// Delay n milliseconds (16 MHz CPU clock)
void delayMs(int n) {
	int32_t i, j;
	for(i = 0 ; i < n; i++)
		for(j = 0 ; j < 3180; i++)
			{} // Do nothing for 1ms
}

void SystemInit(void) {
	// Grant coprocessor access
	// This is required since TM4C123G has a floating point coprocessor
	SCB->CPACR |= 0x00f00000;
}
