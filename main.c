#include <msp430.h>

/**
 * initialization of the msp430.
 * 0.close watch dog
 * 1.system clock
 * 2.Port Out(LED1 & LED2)
 * 3.Port In(KEY)
 * 4.Serial Port in Interrupt mode
 * 5.Timer A for PWN out
 * 6.Timer B for System and Delay
 * 7.ADC 10
 */
void All_Init(void)
{
    WDTCTL = WDTPW | WDTHOLD;       // stop watchdog timer

    BCSCTL3 = LFXT1S1 + XCAP0 + XT2OF + LFXT1OF;
    BCSCTL2 = SELM_0 + DIVM_0 + DIVS_1;             // MCLK to DCOCLK with /1 and SMCLK to DCOCLK with /2
    BCSCTL1 = XT2OFF + 0;

    P1DIR |= 0x01;                  // configure P1.0 as output
}

/**
 * main
 */
void main(void)
{
    //Init clock and peripherals
    All_Init();

	volatile unsigned int i;		// volatile to prevent optimization

	while(1)
	{
		P1OUT ^= 0x01;				// toggle P1.0
		for(i=10000; i>0; i--);     // delay
	}
}
