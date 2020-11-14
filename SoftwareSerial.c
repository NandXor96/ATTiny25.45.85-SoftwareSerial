#include <avr/io.h>
#include <avr/interrupt.h>
#include "SoftwareSerial.h"

volatile uint8_t rxBitCounter = 0; // Counter for received Bits
volatile uint8_t rxChar;           // Received Char
volatile uint8_t rxPar = 1;        // Parity Flag
volatile uint8_t rxFin = 1;        // Char received

volatile uint16_t txReg = 0x00;    // tx Register
volatile uint8_t txBitCounter = 0; // Counter for sent Bits

void txRxInit()
{

    DDRB |= _BV(TX_PIN);  // Set TX_PIN as Ausgang setzen
    PORTB |= _BV(TX_PIN); // Set TX_Pin HIGH
    MCUCR |= _BV(ISC01);  // External interrupt at falling edge
    GIMSK |= _BV(INT0);   // Activate external interrupt at int0
    TCCR0A = _BV(WGM01);  // Set Timer0 to CTC Mode
    TCCR0B = _BV(CS01);   // Set clockdivider to 8 (8MHz / 8 = 1MHz)
    OCR0A = BIT_TIME;     // Set Timer

    sei(); // Set interrupt flag in SREG
}

/**
 * Function to calculate parity (Odd)
 */
uint8_t parity(char n)
{
    uint8_t p = 0;

    for (; n; n >>= 1)
    {
        p = (n & 1) ? !p : p;
    }

    return p;
}

/**
 * Function to send single char
 */
void txChar(char send)
{
    while (!rxFin)
        ;

    if (!txReg)
    {
#ifdef PARITY
        txReg = (send << 1) | (parity(send) << 9) | (1 << 10);
#else
        txReg = (send << 1) | (1 << 9);
#endif
        TIMSK |= _BV(OCF0A); // Activate Interrupt for TIM0 COMP A
        TIFR |= _BV(OCF0A);  // Reset timer interrupt flag
    }
}

/**
 * Function to send whole string
 */
void txStr(char *string)
{
    while (*string)
    {
        txChar(*string++);

        // wait for transmission
        while (txReg)
            ;
    }
}

/**
 * Interrupt-function INT0 at falling edge (start-bit)
 * Include in main.c via ISR(INT0_vect){rxExtInt();}
 */
void rxExtInt()
{
    if (!(PINB & _BV(RX_PIN)))
    {
        TIFR |= _BV(OCF0A);   // Reset timer interrupt flag
        TIMSK &= ~_BV(OCF0A); // Deactivate Interrupt for TIM0 COMP A
        PORTB |= _BV(TX_PIN); // Set TX_PIN HIGH (reset tx)
        rxChar = 0;
        rxBitCounter = 0;
        rxPar = 1;
        rxFin = 0;
        txBitCounter = 0;
        txReg = 0;
        TCNT0 = 0;              // Reset counter
        OCR0A = BIT_TIME * 1.2; // multiply timer by 1.2 to get more into the middle of the second bit
        TIMSK |= _BV(OCF0A);    // Activate Interrupt TIM0 COMP A
        GIMSK &= ~_BV(INT0);    // Deactivate this interrupt function
    }
}

/**
 * Interrupt-function for TIM0 COMP A
 * Include in main.c via ISR(TIM0_COMPA_vect){rxTxTimerInt();}
 */
void rxTxTimerInt()
{
    /**
     * RX Part
     */
    if (!rxFin)
    {
        // First 8 bits will be stored in reverse in rxChar
        if (rxBitCounter < 8 && (PINB & _BV(RX_PIN)))
        {
            rxChar |= _BV(rxBitCounter);
            rxPar = !rxPar; // toggle parity
        }
#ifdef PARITY
        // Check Parity
        else if (rxBitCounter == 8)
        {
#if PARITY == 1
            if ((rxPar && !(PINB & _BV(RX_PIN))) || (!rxPar && (PINB & _BV(RX_PIN))))
#else
            if (rxPar && (PINB & _BV(RX_PIN)))
#endif
                // False Parity - Do not accept received data.
                rxChar = 0;
        }

        else if (rxBitCounter == 9)
#else
        else if (rxBitCounter == 8)
#endif
        {
            rxFin = 1;            // Set rxFin
            TIMSK &= ~_BV(OCF0A); // Deactivate this interrupt
            GIFR |= _BV(INTF0);   // Reset external interrupt flag
            GIMSK |= _BV(INT0);   // Activate external interrupt
        }

        if (rxBitCounter == 0)
        {
            OCR0A = BIT_TIME; // Set timer to 1 * BIT_Time for alle the following bits
        }

        rxBitCounter++;
    }

    /**
     * TX Part
     */
    else if (txReg != 0)
    {
        // Send txReg in reverse
        if ((txReg >> txBitCounter) & 0x01)
        {
            PORTB |= _BV(TX_PIN);
        }
        else
        {
            PORTB &= ~_BV(TX_PIN);
        }
        txBitCounter++;

        // If whole txReg is sent
#ifdef PARITY
        if (txBitCounter > 10)
#else
        if (txBitCounter > 9)
#endif
        {
            txBitCounter = 0;     // Reset txBitCounter
            txReg = 0;            // Reset txReg
            TIMSK &= ~_BV(OCF0A); // Deactivate this interrupt
        }
    }
}