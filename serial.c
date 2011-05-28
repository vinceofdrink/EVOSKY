/*
 * serial.c
 *
 *  Created on: 28 mai 2011
 *      Author: vince
 */
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

serial0_init(unsigned long baud)
{
	  unsigned char baudrateDiv;

	  baudrateDiv = (unsigned char)((F_CPU+(baud*8L))/(baud*16L)-1);

	  UBRR0H = baudrateDiv >> 8;
	  UBRR0L = baudrateDiv;

	  // ACTIVER LA RECEPTION ET L'EMISSION
	  UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	  // SET 1 STOP BIT ET 8BIT MESSAGE
	  UCSR0C = (0 << USBS0) | (3 << UCSZ0);

	  // ENABLE ITERRUPT FOR RECE
	  //SB_HIGH(UCSR0B,RXCIE0);

}

/*
 *
PE1 PDO/TXD0 (Programming Data Output or UART0 Transmit Pin)
PE0 PDI/RXD0 (Programming Data Input or UART0 Receive Pin)
 */
void serial0_writechar( unsigned char data ) {
 loop_until_bit_is_set(UCSR0A, UDRE);
  UDR0 = data;
}



serial1_init(unsigned long baud)
{
		unsigned char baudrateDiv;
		baudrateDiv = (unsigned char)((F_CPU+(baud*8L))/(baud*16L)-1);
	  	  //GESTION SERIAL 2
	  	UBRR1H = baudrateDiv >> 8;
	    UBRR1L = baudrateDiv;

	    // ACTIVER LA RECEPTION ET L'EMISSION
	    UCSR1B = (1 << RXEN1) | (1 << TXEN1);

	    // SET 1 STOP BIT ET 8BIT MESSAGE
	    UCSR1C = (0 << USBS0) | (3 << UCSZ0);

	    // ENABLE ITERRUPT FOR RECE
	    //SB_HIGH(UCSR1B,RXCIE1);
}
/*

 PD3 INT3/TXD1(1) (External Interrupt3 Input or UART1 Transmit Pin)
PD2 INT2/RXD1(1) (External Interrupt2 Input or UART1 Receive Pin
 */
void serial1_writechar( unsigned char data ) {
 loop_until_bit_is_set(UCSR1A, UDRE1);
  UDR1 = data;
}
