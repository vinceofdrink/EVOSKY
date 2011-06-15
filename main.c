#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "macro_atmega.h"
#include "write_ppm.h"
#include "serial.h"
#include "royal_evo.h"
#include "FrSky.h"
// PB5 Sortie PPM

// TIMER1 overflow interrupt service routine
// occurs every 0.5 seconds
//const unsigned char ATBD[] PROGMEM = "ATBD";

#define MODE_FLY  		1
#define MODE_USB_ROYAL	2
#define MODE_USB_FRSKY	3



void  reset_atmega128(void);
void USART_Transmit( unsigned char );
volatile unsigned char toggle_port=1;
volatile unsigned char speed=0;



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
	unsigned char mode;	//DETERMINE WICH MODE WE RUN THE PROGRAM

	//RIGHT NOW I ASSIGN BY DEFAULT THE MODE_FLY WICH IS THE NORMAL MODE
	mode=MODE_FLY;
	signed int test;


	switch(mode)
	{
		case MODE_FLY:
			init_FrSky();				//Initialise Cnx with Frsky Module
			init_royal();				//Initialise Cnx with Royal Evo
			while(TRUE)
			{

				if(royal_trame_ok())	//Test if we have a valid data from EVO
				{
					//DECODE PART AND PPM
					 decode_evo_data(); //Decode data from evo and fill the chanel value into ppm module
					 write_ppm();		//Write a PPM Signal (Asynchrone process)
					 read_FrSky();		//Read Data receive from FRSKY
					//HANDLING TELEMETRY DATA

					 set_royal_evo_rssi(get_FrSky_rssi1());	//ASSIGN rssi
					 test++;
					 set_evo_telemetry(0,UNIT_V,test,0);  //FILL THE VALUE

					 send_evo_telemetry();	//SEND THE TELEMETRY TO ROYAL EVO
				}
			}
		break;

		case MODE_USB_ROYAL:

		break;

		case MODE_USB_FRSKY:
		break;

	}


return 1;
}

void bind_and_ready()
{

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

void test(void)
{
	//LED PORT SET AS AN OUTPUT

		SET_PORT_AS_OUTPUT(A,7);
		//SET_PORT_AS_OUTPUT(B,5);

		serial0_init(115200);
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


		SET_PORT_HIGH(A,7);
	while (1)
	   {

		//TOOGLE_PORT(A,7);


		//SET_PORT_HIGH(A,7);
	    _delay_ms(10);




	    if(serial1_NewData() )
	    {
	    	serial1_writechar(serial1_readchar());
	    	TOOGLE_PORT(A,7);
	    }
	    if(serial0_NewData() )
	       	serial0_writechar(serial0_readchar());


	    write_ppm();




	    //write_ppm();
	    //SET_PORT_LOW(A,7);
	       //_delay_ms(100);
	    }

}


