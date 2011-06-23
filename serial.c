/*
 * serial.c
 *
 *  Created on: 28 mai 2011
 *      Author: vince
 *
 *      PE1 PDO/TXD0 (Programming Data Output or UART0 Transmit Pin)
 *      PE0 PDI/RXD0 (Programming Data Input or UART0 Receive Pin)
 *
 *      PD3 INT3/TXD1(1) (External Interrupt3 Input or UART1 Transmit Pin)
 *      PD2 INT2/RXD1(1) (External Interrupt2 Input or UART1 Receive Pin
 *
 */
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "macro_atmega.h"




#define SERIAL_C_
#include "serial.h"

void serial0_init(unsigned long baud)
{
	  unsigned int baudrateDiv;
	  cli();
	  //baudrateDiv = (unsigned char)((F_CPU+(baud*8L))/(baud*16L)-1);
	  baudrateDiv =F_CPU/16/baud-1;
	  UBRR0H =(unsigned char) baudrateDiv >> 8;
	  UBRR0L =(unsigned char) baudrateDiv;

	  // ACTIVER LA RECEPTION ET L'EMISSION
	  UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	  // SET 1 STOP BIT ET 8BIT MESSAGE
	  UCSR0C = (0 << USBS0) | (3 << UCSZ0);

	  // ENABLE ITERRUPT FOR RECE
	  SB_HIGH(UCSR0B,RXCIE0);
	  sei();

}
void serial_0_change_rate(unsigned long baud)
{
	unsigned int baudrateDiv;
		  cli();
		  //baudrateDiv = (unsigned char)((F_CPU+(baud*8L))/(baud*16L)-1);
		  baudrateDiv =F_CPU/16/baud-1;
		  UBRR0H =(unsigned char) baudrateDiv >> 8;
		  UBRR0L =(unsigned char) baudrateDiv;
		  sei();
}

/**
 * @todo Need to do something here
 */
void serial0_close(void)
{

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
void serial0_writestring(char * str) // Envoi une chaine de caractère sur l'UART0
{
	while (str[0]!=0)
	{
		serial0_writechar(str[0]);
	    str++;
	}
}
unsigned char serial0_readchar()
{
	unsigned char tmp=serial0_input[serial0_input_readct];
		serial0_input_readct++;
		if(serial0_input_readct==SERIAL0_BUFFER)
			serial0_input_readct=0;
		return tmp;
}

ISR(USART0_RX_vect)
{
	//READ UDR0
   // Code to be executed when the USART receives a byte here
	serial0_input[serial0_input_writect++]=UDR0;
	//Reset TIMER0 that when overflow trigger an interrupt into RoyalEvo.c that let us presume that

	//Hack Specific to this project
	//We Reset to 0 TIMER0 TO POSTPONE OVERFLO Until We have complete a RoyalEvo Frame
	ooTIMER0_CT=0;

	//Handle Circular Buffer
	if(SERIAL0_BUFFER==serial0_input_writect)
		serial0_input_writect=0;


}




void serial1_init(unsigned long baud)
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
	    SB_HIGH(UCSR1B,RXCIE1);
}
/**
 * @todo Need to do something here
 */
void serial1_close(void)
{

}
/*

 PD3 INT3/TXD1(1) (External Interrupt3 Input or UART1 Transmit Pin)
PD2 INT2/RXD1(1) (External Interrupt2 Input or UART1 Receive Pin
 */
void serial1_writechar( unsigned char data ) {
 loop_until_bit_is_set(UCSR1A, UDRE1);
  UDR1 = data;
}
void serial1_writestring(char * str) // Envoi une chaine de caractère sur l'UART0
{
	while (str[0]!=0)
	{
		serial1_writechar(str[0]);
	    str++;
	}
}
unsigned char serial1_readchar(void)
{
	unsigned char tmp=serial1_input[serial1_input_readct];
	serial1_input_readct++;
	if(serial1_input_readct==SERIAL1_BUFFER)
		serial1_input_readct=0;
	return tmp;
}
ISR(USART1_RX_vect)
{
	//READ UDR1
   // Code to be executed when the USART receives a byte here
	serial1_input[serial1_input_writect++]=UDR1;


	//Gestion du débordement du buffer
	if(SERIAL1_BUFFER==serial1_input_writect)
		serial1_input_writect=0;
}
