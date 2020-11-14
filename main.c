/**
 * Demo program to toggle a LED at PB1 from Serial Connection on an ATTiny 25/45/85
 * 
 * Default values are:
 * BAUD: 9600
 * TX: PB0
 * RX: PB2
 * PARITY: ODD
 * 
 * Sending '1' to the attiny turns the LED on and sends "On" back
 * Sending '0' to the attiny turns the LED off and sends "Off" back
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "SoftwareSerial.h"

uint8_t sendFlag = 1;

#define OSCI_TUNE -2 // OSCI correction factor
#define LED_PIN PB1  // LED-Output PIN

int main(void)
{
  /**
   * Setup
   */
  OSCCAL += OSCI_TUNE;  // Calibrate internal oscillator
  DDRB |= _BV(LED_PIN); // Set LED Pin to output

  txRxInit();

  /**
   * Loop
   */
  while (1)
  {
    if (rxFin && sendFlag)
    {
      if (rxChar == '1')
      {
        PORTB |= _BV(LED_PIN);
        txStr("On\n");
      }
      else if (rxChar == '0')
      {
        PORTB &= ~_BV(LED_PIN);
        txStr("Off\n");
      }
      sendFlag = 0;
    }

    if (!rxFin)
      sendFlag = 1;
  }

  return 0;
}

ISR(INT0_vect)
{
  rxExtInt();
}

ISR(TIM0_COMPA_vect)
{
  rxTxTimerInt();
}