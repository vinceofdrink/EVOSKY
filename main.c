#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "macro_atmega.h"
#include "write_ppm.h"


// TIMER1 overflow interrupt service routine
// occurs every 0.5 seconds
//const unsigned char ATBD[] PROGMEM = "ATBD";

void  reset_atmega128(void);
void USART_Transmit( unsigned char );
volatile unsigned char toggle_port=1;
volatile unsigned char speed=0;

ISR(TIMER0_OVF_vect)
{
    
    speed++;
    if(speed==200)
        speed=0;
    
    toggle_port=!toggle_port;
    if(toggle_port)
        SET_PORT_HIGH(A,0);
    else
        SET_PORT_LOW(A,0);

    ooTIMER0_CT=speed;
}

#define UART_BAUD_RATE 19200
int main(void)
{
   
	SET_PORT_AS_OUTPUT(A,7);

	SET_PORT_AS_OUTPUT(E,1);

	while (1)
	   {

	   }
    //reset_atmega128();
    //DDRA=0xFF;
   // SET_PORT_AS_OUTPUT(A,0);
   // ooTIMER0_SCALE_1024;
    //ooTIMER0_OVERFLOW_ON;
    //sei();
    //SET_PORT_AS_OUTPUT(D,3);
    //SET_PORT_AS_OUTPUT(E,1);

  unsigned char baudrateDiv;

  baudrateDiv = (unsigned char)((F_CPU+(UART_BAUD_RATE*8L))/(UART_BAUD_RATE*16L)-1);

  UBRR0H = baudrateDiv >> 8;
  UBRR0L = baudrateDiv;

  // ACTIVER LA RECEPTION ET L'EMISSION
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);

  // SET 1 STOP BIT ET 8BIT MESSAGE
  UCSR0C = (0 << USBS0) | (3 << UCSZ0);

  // ENABLE ITERRUPT FOR RECE
  //SB_HIGH(UCSR0B,RXCIE0);

  	  //GESTION SERIAL 2
  	UBRR1H = baudrateDiv >> 8;
    UBRR1L = baudrateDiv;

    // ACTIVER LA RECEPTION ET L'EMISSION
    UCSR1B = (1 << RXEN1) | (1 << TXEN1);

    // SET 1 STOP BIT ET 8BIT MESSAGE
    UCSR1C = (0 << USBS1) | (3 << UCSZ1);

    // ENABLE ITERRUPT FOR RECE
    //SB_HIGH(UCSR1B,RXCIE1);
while (1)
   {




	SET_PORT_HIGH(A,7);
    _delay_ms(100);


    USART1_Transmit('V');
    USART0_Transmit('D');
    SET_PORT_LOW(A,7);
       _delay_ms(100);
    }


return 1;
}

/*
 *
PE1 PDO/TXD0 (Programming Data Output or UART0 Transmit Pin)
PE0 PDI/RXD0 (Programming Data Input or UART0 Receive Pin)
 */
void USART0_Transmit( unsigned char data ) {
 loop_until_bit_is_set(UCSR0A, UDRE);
  UDR0 = data;
}


/*

 PD3 INT3/TXD1(1) (External Interrupt3 Input or UART1 Transmit Pin)
PD2 INT2/RXD1(1) (External Interrupt2 Input or UART1 Receive Pin
 */
void USART1_Transmit( unsigned char data ) {
 loop_until_bit_is_set(UCSR1A, UDRE1);
  UDR1 = data;
}

void  reset_atmega128(void)
{
   // SET ALL THE PORT TO INPUT WITH PULLUP
    cli();
    RESET_PORT(A);
    RESET_PORT(B);
    RESET_PORT(C);
    RESET_PORT(D);
    RESET_PORT(E);
    RESET_PORT(F);
    RESET_PORT(G);
    ooTIMER0_STOP;
}




