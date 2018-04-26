#include <msp430.h>

/**
 * initialization of the msp430.
 * 0.close watch dog
 * 1.system clock
 * 2.Set P1 and P2
 *   P1.0	LED1
 *   P1.1	UART1 TX
 *   P1.2	UART1 RX
 *   P1.3	KEY
 *   P1.4
 *   P1.5
 *   P1.6	LED2
 *   P1.7
 *   P2.0
 *   P2.1	TA1.1
 *   P2.2
 *   P2.3
 *   P2.4
 *   P2.5
 * 3.Serial Port in Interrupt mode
 * 5.Timer A for PWN out
 * 6.Timer B for System and Delay
 * 7.ADC 10
 */

#include <msp430.h>

#define PWM_Frequence		1000	//PWM频率，单位是Hz，最低频率123Hz（大约是这个数）
#define PWM_Duty_Cycle		75		//PWM占空比，0-100，最好不要取太接近0或接近100

unsigned int counter_5ms = 0;

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

	//////////////////////////////////////////////////////////////

	//LED
	P1DIR |= BIT0 + BIT6;                     // P1.0 P1.6 output

	//////////////////////////////////////////////////////////////

	//Set clock
	//ACLK 12k, MCLK 16M, SMCLK 8M
	if (CALBC1_16MHZ==0xFF)                   // If calibration constant erased
	{
		while(1);                             // do not load, trap CPU!!
	}
	DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_16MHZ;                   // Set range
	BCSCTL1 &= (0x0F | XT2OFF);               // Clear bit 4-6
	BCSCTL1 |= XT2OFF;
	DCOCTL = CALDCO_16MHZ;                    // Set DCO step + modulation*/

	BCSCTL3 = LFXT1S1 + XCAP0 + XT2OF + LFXT1OF;    // Use VLOCLK for ACLK
	BCSCTL2 = SELM_0 + DIVM_0 + DIVS_1;             // MCLK to DCOCLK with /1 and SMCLK to DCOCLK with /2

	//////////////////////////////////////////////////////////////

	//Timer0_A 5ms Interrupt
	CCTL0 = CCIE;                             	// CCR0 interrupt enabled
	CCR0 = 40000;								// 5ms中断周期
	TACTL = TASSEL_2 + MC_2;                  	// SMCLK, contmode

	//////////////////////////////////////////////////////////////

	unsigned int pwm_freq_tmp = 8000000/PWM_Frequence-1;						//CCR0的计数值代表周期
	unsigned int pwm_duty_cycle_tmp = (PWM_Duty_Cycle/100.f) * pwm_freq_tmp;	//CCR1的比较值代表占空比

	//Timer1_A PWM Out
	P2DIR |= BIT1;                            	// P2.1 output
	P2SEL |= BIT1;                            	// P2.1 for TA1.1 output
	P2SEL2 = 0;									// Select default function for P2.1
	TA1CCR0 = pwm_freq_tmp;         	 		// PWM Period
	TA1CCTL1 = OUTMOD_7;                        // CCR1 reset/set
	TA1CCR1 = pwm_duty_cycle_tmp;               // CCR1 PWM duty cycle
	TA1CTL = TASSEL_2 + MC_1;                  	// SMCLK, up mode

	//////////////////////////////////////////////////////////////

	__bis_SR_register(GIE);       // Enable interrupt

	//////////////////////////////////////////////////////////////

	unsigned int led_counter_1 = 0;
	unsigned int led_counter_2 = 0;

	while(1)
	{
		if(counter_5ms >= 1)	//5ms周期
		{
			counter_5ms = 0;

			///////////////////////////////////////////////////////

			led_counter_1++;
			if(led_counter_1 > 50)
			{
				led_counter_1 = 0;
				P1OUT ^= 0x01;      	// Toggle P1.0
			}

			led_counter_2++;
			if(led_counter_2 > 50)
			{
				led_counter_2 = 0;
				P1OUT ^= 0x40;      	// Toggle P1.6
			}

			////////////////////////////////////////////////////////




		}
	}
}

// Timer A0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A_CC0 (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{
	counter_5ms++;

	CCR0 += 40000;                            // Add Offset to CCR0			5ms中断周期
}
