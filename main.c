#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "macro_atmega.h"
#include "write_ppm.h"
#include "serial.h"

// PB5 Sortie PPM

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
/*
ISR(TIMER1_COMPA_vect)
{
	SET_PORT_HIGH(A,7);
	OCR1A=ooTIMER1_CT+100;
}
*/
#define UART_BAUD_RATE 19200
int main(void)
{
   
	SET_PORT_AS_OUTPUT(A,7);
	SET_PORT_AS_OUTPUT(B,5);

	serial0_init(19200);
	serial1_init(115200);

	//SET_PORT_AS_OUTPUT(E,1);

	init_ppm();
	g_chanel1[0]=0;
	g_chanel1[1]=-500;
	g_chanel1[2]=500;

    //reset_atmega128();
    //DDRA=0xFF;
   // SET_PORT_AS_OUTPUT(A,0);
   // ooTIMER0_SCALE_1024;
    //ooTIMER0_OVERFLOW_ON;
    //sei();
    //SET_PORT_AS_OUTPUT(D,3);
    //SET_PORT_AS_OUTPUT(E,1);
	if(0==1)
	{
	OCR1A=0x1000;
	ooTIMER1_NORMAL_MODE;
	ooTIMER1_CT = 0x00;
	//TIMSK = 1 << OCIE1A ;
	SB_HIGH(TIMSK,OCIE1A);
	ooTIMER1_COMP_A_TOOGLE;
	ooTIMER1_SCALE_8;
	}


	sei();



while (1)
   {




	//SET_PORT_HIGH(A,7);
    _delay_ms(100);

    serial0_writechar('V');
    serial0_writechar('I');
    serial0_writechar('C');
    serial1_writechar('E');
    serial1_writechar('N');
    serial1_writechar('T');
    write_ppm();




    //write_ppm();
    //SET_PORT_LOW(A,7);
       _delay_ms(100);
    }


return 1;
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




