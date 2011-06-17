#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "macro_atmega.h"
#include "write_ppm.h"
#include "serial.h"
#include "royal_evo.h"
#include "FrSky.h"

// OUTPUT USED ON ATMEGA128

// PB5 Sortie PPM  -> SHOULD BE CONNECT TO FRSKY INPUT PPM //  1K Ohm Resistor is problably welcome between AVR-FRSKY PPM CNX

// PA7 ON BOARD LED -> USE FOR THE STATUT LED ON BOARD (I USE A 1K Ohm Resistor)
// PA6 SWITCH THAT SHOULD CONNECT TO GROUND AND ALLOW MODE SELECTION  SHOULD BE PLACED ON THE OUTSIDE OF ROYAL EVO


#define OPTION_WATCHDOG	1  //Use WATCHDOG TO RESET IF THE PROGRAM IS PRESUMED BRICK
#define OPTION_BROWN_OUT 0 //SET TO 1 IF WE WANT TO CHECK BROWN_OUT OPTION TO DETECT VOLTAGE GARBAGE
// TIMER1 overflow interrupt service routine
// occurs every 0.5 seconds
//const unsigned char ATBD[] PROGMEM = "ATBD";


#define MODE_FLY  		1
#define MODE_USB_ROYAL	2
#define MODE_USB_FRSKY	3

int main(void)
{
	unsigned char mode;	//STORE WICH MODE WE RUN THE PROGRAM
	unsigned char standard_boot=TRUE;
	//WE TRY TO DETECT WHAT KING OF POWER-ON WE ARE FACING

	//I plan to make a visual alarm that would reflect BROWN-OUT and WATCHDOG event because
	//There are real threat for a cool and nice FLY :)

	#if OPTION_WATCHDOG==1
	//WATCHDOG ISSUE (NOT COOL MEANS A SOFTWARE BUG OR VOLTAGE GARBAGE AS STUCK
	//THE PROGRAM MORE THAT 120MS THATS ABOUT 4/5 PPM Cycle that have not been send (I reset Watchdog on every PPM CALL)
	if( READ_BIT(MCUCSR,WDRF))
	{
		// At least here i will try to bind again with royal evo even if i have to skip
		// The negotiation pahse that maybe is aleready done and royal is problably sending me a Channel Position

		standard_boot=FALSE;
		SB_LOW(MCUCSR,WDRF);
	}
	#endif

	#if OPTION_BROWN_OUT==1
		//BROWN-OUT
		// Means we have poor power source for our avr and Brown-out as trigered a reset
		if( READ_BIT(MCUCSR,BORF))
		{
			//Same scenario as watchdog event
			standard_boot=FALSE;
			SB_LOW(MCUCSR,WDRF);
		}
	#endif
	//Reset BY USER PRESSING RESET BUTTON
	if( READ_BIT(MCUCSR,EXTRF))
	{
		//Nothing special
	}

	//Reset from Normal Power On
	if( READ_BIT(MCUCSR,PORF))
	{
		//nothing special
	}


	//RIGHT NOW I ASSIGN BY DEFAULT THE MODE_FLY WICH IS THE NORMAL MODE
	mode=MODE_FLY;

	//WE SUPPOSE TROUBLE ON BOOT  (WatchDog or Brown-out) FORCE the mode to MODE_FLY
	//TO BEGIN AS FAST AS POSSIBLE THE PPM EMISSION
	if(!standard_boot)
		mode=MODE_FLY;
	signed int test=0;


	switch(mode)
	{
		case MODE_FLY:
			Init_FrSky();				//Initialise Cnx with Frsky Module
			init_royal(standard_boot);	//Initialise Cnx with Royal Evo (standard boot means we will wait for real negotiation with royal evo otherwise if we cannot akwnoledge Royal evo we skip try several nego and then start directly to listening to channel position stream
			init_ppm();					//Initialise PPM Writer

			//We activate the watchdog that will trigger a reset if wdt_reset() is not call every 120MS
			#if OPTION_WATCHDOG==1
				wdt_enable(WDTO_120MS);
			#endif
			while(TRUE)
			{

				if(royal_trame_ok())	//Test if we have a valid data from EVO
				{
					#if OPTION_WATCHDOG==1
					wdt_reset();
					#endif
					//DECODE PART AND PPM
					 decode_evo_data(); //Decode data from evo and fill the chanel value into ppm module
					 write_ppm();		//Write a PPM Signal (Asynchrone process)
					 Read_FrSky();		//Read Data receive from FRSKY
					//HANDLING TELEMETRY DATA
					 if(NewUserDataFrSky())
					 {
						 //HANDLE USER SERIAL STREAM FROM FRSKY (DATA SEND FROM RX)
						 unsigned char tmp;
						 tmp=ReadUserDataFrSky();
						 //DO NOTHING RIGHT NOW BUT I WILL DEFINE A PROTOCOL TO REMOTLY FROM RX ASSIGN DATA TO TELEMETRY CHANNEL
					 }
					 set_royal_evo_rssi(get_FrSky_rssi_up_link());	//ASSIGN rssi
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


