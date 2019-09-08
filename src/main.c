/*
 * main.c
 */

#include  "msp430g2553.h"

#define     LED1                     BIT0
#define     LED2                     BIT6
#define     SERVO_L                  BIT4
#define     SERVO_R                  BIT7

#define     TIMER_PWM_MODE        0
#define     TIMER_UART_MODE       1
#define     TIMER_PWM_PERIOD      20000
#define     TIMER_PWM_OFFSET      200
#define     TIMER_PULSE_DEFAULT   1000

unsigned long i;
int sign = 0;

void ConfigureTimerPwm(void);
void InitializeClocks(void);


void main(void)
{

  WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer

  InitializeClocks();
  ConfigureTimerPwm();

  __enable_interrupt();                     // Enable interrupts.

  /* Main Application Loop */
  P1DIR = SERVO_L + SERVO_R + LED1 + LED2;
  while(1)
  {
      P1OUT ^= LED1 + LED2;
      for(i=0; i < 100000; i++) {
      }
      if (sign == 0) {
          TACCR1 = 1000; // 1ms
          TACCR2 = 1000; // 1ms
          sign = 1;
      } else {
          TACCR1 = 2000; // 2ms duty
          TACCR2 = 2000; // 2ms duty
          sign = 0;
      }
  }
}

/*
 * TIMER A0: set pulse width duty cycle
 * TIMER A1: trigger on SERVO_R
 * TIMER A2: trigger on SERVO_L
 */
void ConfigureTimerPwm(void)
{

  TACCR0 = TIMER_PWM_PERIOD;                // Value to count to (Up Mode), 20ms
  TACTL = TASSEL_2 | MC_1;                  // TACLK = SMCLK, Up mode. TAR counts to TACCR0.
  TACCTL0 = CCIE;                           // Capture/compare interrupt enable

  TACCTL1 = CCIE + OUTMOD_3;                // Set/Reset. Output is set at TACCR1, Reset at TACCR0.
  TACCR1 = TIMER_PULSE_DEFAULT;                               // Capture/Compare register

  TACCTL2 = CCIE + OUTMOD_3;
  TACCR2 = TIMER_PULSE_DEFAULT;
}


// Timer A0 interrupt service routine
// Turn ON SERVO_L and SERVO_R at start of cycle.
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    P1OUT |= SERVO_L + SERVO_R;
    TACCTL0 &= ~CCIFG;

}

/*
* Timer A1, A2 interrupt service routine
* Turn OFF SERVO_L when TACCR0 > TACCR1
* Turn OFF SERVO_R when TACCR0 > TACCR2
*/
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A1_ISR(void)
{
  switch (__even_in_range(TAIV, 10)) {  // Example on EW430_CompilerReference.pDF p75
  // SERVO_L
  case TA0IV_TACCR1: P1OUT &= ~SERVO_L;
                     TACCTL1 &= ~CCIFG;
                     break;
  // SERVO_R
  case TA0IV_TACCR2: P1OUT &= ~SERVO_R;
                     TACCTL2 &= ~CCIFG;
                     break;
  // Overflow
  case TA0IV_TAIFG:  break;
  }
}

void InitializeClocks(void)
{

  BCSCTL1 = CALBC1_1MHZ;                    // Set to 0xFF. /8x, 15.25MHz
  DCOCTL = CALDCO_1MHZ;
  BCSCTL2 &= ~(DIVS_3);                     // SMCLK = DCO = 1MHz
}


