#include <msp430.h>

/**
 *   P1.0	LED1
 *   P1.1	UART1 TX
 *   P1.2	UART1 RX
 *   P1.3	KEY
 *   P1.4	(SMCLK)
 *   P1.5
 *   P1.6	LED2
 *   P1.7	ADC10.7
 *   P2.0
 *   P2.1	TA1.1 PWM OUT
 *   P2.2
 *   P2.3
 *   P2.4
 *   P2.5
 */

#include <msp430.h>
#include <stdio.h>
#include <string.h>

// To set the use of printf in msp430
// Please see the webpage below: http://processors.wiki.ti.com/index.php/Printf_support_for_MSP430_CCSTUDIO_compiler
#define UART_PRINTF
#ifdef UART_PRINTF
int fputc(int _c, register FILE *_fp);
int fputs(const char *_ptr, register FILE *_fp);
#endif

#define u8 unsigned char
#define u16 unsigned int
#define u32 unsigned long

u16 counter_5ms = 0;
u16 counter_500ms = 0;

u32 Pwm_Frequence = 1000;	//PWM频率，单位是Hz，最低频率123Hz（大约是这个数),最高0.8M就差不多了（这个时候占空比已经不准了）
u8 Pwm_Duty_Cycle = 75;		//PWM占空比，0-100，最好不要取太接近0或接近100
u16 adc_in = 0;

/* *****************************
 * UART_Send_Byte
 * Input:
 * byte 1个待发送字节
 * *****************************/
void UART_Send_Byte(u8 byte)
{
	while (!(IFG2&UCA0TXIFG));                	// USCI_A0 TX buffer ready?
	UCA0TXBUF = byte;                    		// TX -> RXed character
}

/* *****************************
 * PWN_Set
 * Input:
 * freq 频率，单位Hz
 * duty_cyrcle 占空比，0-100
 * *****************************/
void PWN_Set(u16 freq,u8 duty_cyrcle)
{
	Pwm_Frequence = freq;
	Pwm_Duty_Cycle = duty_cyrcle;

	u16 pwm_freq_tmp = 8000000/Pwm_Frequence-1;						//CCR0的计数值代表周期
	u16 pwm_duty_cycle_tmp = (Pwm_Duty_Cycle/100.f) * pwm_freq_tmp;	//CCR1的比较值代表占空比

	TA1CCR0 = pwm_freq_tmp;         	 		// PWM Period
	TA1CCTL1 = OUTMOD_7;                        // CCR1 reset/set
	TA1CCR1 = pwm_duty_cycle_tmp;               // CCR1 PWM duty cycle
}

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 	// Stop WDT

	//////////////////////////////////////////////////////////////

	//LED
	P1DIR |= BIT0 + BIT6;                     	// P1.0 P1.6 output

	//////////////////////////////////////////////////////////////

	//Set clock
	//ACLK 12k, MCLK 16M, SMCLK 8M
	if (CALBC1_16MHZ==0xFF)                   	// If calibration constant erased
	{
		while(1);                             	// do not load, trap CPU!!
	}
	DCOCTL = 0;                               	// Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_16MHZ;                   	// Set range
	BCSCTL1 &= (0x0F | XT2OFF);               	// Clear bit 4-6
	BCSCTL1 |= XT2OFF;
	DCOCTL = CALDCO_16MHZ;                    	// Set DCO step + modulation*/

	BCSCTL3 = LFXT1S1 + XCAP0 + XT2OF + LFXT1OF;    // Use VLOCLK for ACLK
	BCSCTL2 = SELM_0 + DIVM_0 + DIVS_1;             // MCLK to DCOCLK with /1 and SMCLK to DCOCLK with /2

	//P1DIR |= 0x13;                            // P1.0,1 and P1.4 outputs
	//P1SEL |= 0x11;                            // P1.0,4 ACLK, SMCLK output

	//////////////////////////////////////////////////////////////

	//Timer0_A 5ms Interrupt
	CCTL0 = CCIE;                             	// CCR0 interrupt enabled
	CCR0 = 40000;								// 5ms中断周期
	TACTL = TASSEL_2 + MC_2;                  	// SMCLK, contmode

	//////////////////////////////////////////////////////////////

	//PWM
	u16 pwm_freq_tmp = 8000000/Pwm_Frequence-1;						//CCR0的计数值代表周期
	u16 pwm_duty_cycle_tmp = (Pwm_Duty_Cycle/100.f) * pwm_freq_tmp;	//CCR1的比较值代表占空比

	//Timer1_A PWM Out
	P2DIR |= BIT1;                            	// P2.1 output
	P2SEL |= BIT1;                            	// P2.1 for TA1.1 output
	P2SEL2 = 0;									// Select default function for P2.1
	TA1CCR0 = pwm_freq_tmp;         	 		// PWM Period
	TA1CCTL1 = OUTMOD_7;                        // CCR1 reset/set
	TA1CCR1 = pwm_duty_cycle_tmp;               // CCR1 PWM duty cycle
	TA1CTL = TASSEL_2 + MC_1;                  	// SMCLK, up mode

	//////////////////////////////////////////////////////////////

	//ADC10
	//ADC自动循环采样，并将结果存入adc_in
	ADC10CTL0 = ADC10SHT_2 + ADC10ON + MSC;		// ADC10ON + 循环采样
	ADC10CTL1 = INCH_7 + CONSEQ_2;  			// input A7 + 重复采样模式
	ADC10AE0 |= 0x80;                         	// PA.7 ADC option select
	ADC10DTC0 |= ADC10CT;						// 连续采样
	ADC10DTC1 = 0x01;
	ADC10SA = (unsigned int)&adc_in;
	ADC10CTL0 |= ENC + ADC10SC;					// 开始采样

	//////////////////////////////////////////////////////////////

	//UART

	P1SEL |= BIT1 + BIT2 ;                     	// P1.1 = RXD, P1.2=TXD
	P1SEL2 |= BIT1 + BIT2;
	UCA0CTL1 |= UCSSEL_2;                     	// SMCLK
	UCA0BR0 = 69;                              	// 8MHz 115200
	UCA0BR1 = 0;                              	// 8MHz 115200
	UCA0MCTL = UCBRS2 + UCBRS0;               	// Modulation UCBRSx = 5
	UCA0CTL1 &= ~UCSWRST;                     	// **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          	// Enable USCI_A0 RX interrupt

	//////////////////////////////////////////////////////////////


	__bis_SR_register(GIE);       // Enable interrupt

	//////////////////////////////////////////////////////////////

	while(1)
	{
		if(counter_5ms >= 1)	//5ms周期
		{
			counter_5ms = 0;

			///////////////////////////////////////////////////////

			static u8 led_counter_1 = 0;
			static u8 led_counter_2 = 0;

			led_counter_1++;
			if(led_counter_1 > 50)
			{
				led_counter_1 = 0;
				//P1OUT ^= 0x01;      	// Toggle P1.0
			}

			led_counter_2++;
			if(led_counter_2 > 50)
			{
				led_counter_2 = 0;
				P1OUT ^= 0x40;      	// Toggle P1.6
			}

			////////////////////////////////////////////////////////



		}

		if(counter_500ms >= 100)	//500ms周期
		{
			counter_500ms = 0;

			float voltage = 0.0f;
			voltage = (float)adc_in * 3.3f / 1024.0f;
			u8 send_int = (u8)voltage;
			u8 send_dec = ((u8)(voltage*10.0f)) % 10;

			//UART_Send_Byte(adc_in);	//发送电压采集值，但只能发送低八位
			printf("%d.%d\r\n",send_int,send_dec);
		}

		// 电压实际值 = adc_in / 1024 * 3.3V
		if (adc_in < 0x1FF)							// 低于1/2参考电压，LED1熄灭，高于1/2参考电压时，LED1点亮
			P1OUT &= ~0x01;                       	// Clear P1.0 LED off
		else
			P1OUT |= 0x01;                        	// Set P1.0 LED on
	}
}

// Echo back RXed character, confirm TX buffer is ready first
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	u8 tmp = UCA0RXBUF;
	if(tmp > 99 || tmp == 0)	//防止占空比大于等于100或等于0
		tmp = 50;
	PWN_Set(10000,tmp);	//设置占空比
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
	counter_500ms++;
	CCR0 += 40000;                            // Add Offset to CCR0			5ms中断周期
}

#ifdef UART_PRINTF
int fputc(int _c, register FILE *_fp)
{
	while (!(IFG2&UCA0TXIFG));                	// USCI_A0 TX buffer ready?
	UCA0TXBUF = (unsigned char) _c;             // TX -> RXed character

	return((unsigned char)_c);
}

int fputs(const char *_ptr, register FILE *_fp)
{
	unsigned int i, len;

	len = strlen(_ptr);

	for(i=0 ; i<len ; i++)
	{
		while (!(IFG2&UCA0TXIFG));                	// USCI_A0 TX buffer ready?
		UCA0TXBUF = (unsigned char) _ptr[i];        // TX -> RXed character
	}

	return len;
}
#endif
