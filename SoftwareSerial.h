#include <avr/io.h>
#include <avr/interrupt.h>

#define BIT_TIME 104 // BAUD:BIT_TIME - 9600:104 - 19200:52 - 38400:26
#define TX_PIN PB0   // TX PIN
#define RX_PIN PB2   // RX PIN (INT0)

#define PARITY 1   // 1=ODD, 2=EVEN, DELETE LINE=NONE

volatile uint8_t rxBitCounter; // Zähler für empfangene Bit's
volatile uint8_t rxChar;       // Empfangenes Char
volatile uint8_t rxPar;        // Flag für die Parität
volatile uint8_t rxFin;        // Flag für abgeschlossenes Empfangen

volatile uint16_t txReg;       // Senderegister
volatile uint8_t txBitCounter; // Zähler für zu sendende Bit's

void txRxInit();
uint8_t parity(char n);
void txChar(char send);
void txStr(char *string);
void rxExtInt();
void rxTxTimerInt();