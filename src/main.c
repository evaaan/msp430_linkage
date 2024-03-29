/*
 * main.c
 */

#include  "msp430g2553.h"

#define     LED2                     BIT6
#define     SERVO_L                  BIT4
#define     SERVO_R                  BIT7
#define     ADC_L                    BIT0
#define     ADC_R                    BIT1

#define     TIMER_PWM_PERIOD         20000  // microseconds
#define     SERVO_PULSE_DEFAULT      1500
#define     SERVO_PULSE_MIN          800
#define     SERVO_PULSE_MAX          2200

unsigned long i;
int sign = 0;
unsigned int result[2];

void InitializeTimerPwm(void);
void InitializeClocks(void);
void InitializeADC(void);
void SetServoPulse(int, int);
void ToggleServos(void);

void main(void) {

    WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer
    P1DIR = SERVO_L + SERVO_R + LED2;

    InitializeClocks();
    InitializeTimerPwm();
    InitializeADC();
    __enable_interrupt();

    while(1) {
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & BUSY);
        ADC10SA = (int)result;
        ADC10CTL0 |= ENC + ADC10SC;   // Sampling and conversion ready
        __bis_SR_register(CPUOFF + GIE); // LPM0, ADC10_ISR will force exit


        SetServoPulse(SERVO_PULSE_MIN + result[0]*1.5, SERVO_PULSE_MIN + result[1]*1.5);
    }

}

void ToggleServos(void) {

    while(1) {
          P1OUT ^= LED2;
          for(i=0; i < 100000; i++) {
          }
          if (sign == 0) {
              SetServoPulse(1000, 1000);
              sign = 1;
          } else {
              SetServoPulse(2000, 2000);
              sign = 0;
          }
      }
}

/*
 * Set servos to positions.
 * param pos_left: left servo position
 * param pos_right: right servo position
 */
void SetServoPulse(int pulse_left, int pulse_right) {

    TACCR1 = pulse_left;
    TACCR2 = pulse_right;
}

void InitializeADC(void) {

    ADC10CTL1 = INCH_1 + CONSEQ_1; // P1.1/P1.0, sequence-of-channels
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON + ADC10IE; // 16x clks, on, interrupt enable
    ADC10AE0 |= ADC_L + ADC_R;                  // P1.1/P1.0 Analog Input Enable
    ADC10DTC1 = 0x2; // 2 conversions

}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
    __bic_SR_register_on_exit(CPUOFF);
}

/*
 * TIMER A0: set pulse width duty cycle
 * TIMER A1: trigger on SERVO_L
 * TIMER A2: trigger on SERVO_R
 */
void InitializeTimerPwm(void) {

    TACCR0 = TIMER_PWM_PERIOD;                // Value to count to (Up Mode), 20ms
    TACTL = TASSEL_2 | MC_1;                  // TACLK = SMCLK, Up mode. TAR counts to TACCR0.
    TACCTL0 = CCIE;                           // Capture/compare interrupt enable

    TACCTL1 = CCIE + OUTMOD_3;                // Set/Reset. Output is set at TACCR1, Reset at TACCR0.
    TACCR1 = SERVO_PULSE_DEFAULT;                               // Capture/Compare register

    TACCTL2 = CCIE + OUTMOD_3;
    TACCR2 = SERVO_PULSE_DEFAULT;
}

/*
 * Timer A0 interrupt service routine
 * Turn ON SERVO_L and SERVO_R at start of cycle.
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void) {

    P1OUT |= SERVO_L + SERVO_R;
    TACCTL0 &= ~CCIFG;
}

/*
* Timer A1, A2 interrupt service routine
* Turn OFF SERVO_L when TACCR0 > TACCR1
* Turn OFF SERVO_R when TACCR0 > TACCR2
*/
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A1_ISR(void) {

    switch (__even_in_range(TAIV, 10)) {  // Example on EW430_CompilerReference.pdf p75
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

void InitializeClocks(void) {

    BCSCTL1 = CALBC1_1MHZ;                    // Set to 0xFF. /8x, 15.25MHz
    DCOCTL = CALDCO_1MHZ;
    BCSCTL2 &= ~(DIVS_3);                     // SMCLK = DCO = 1MHz
}
