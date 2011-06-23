/*
 * PROJECT INTEND TO WORK ON ATMEGA128 WITH F_CPU AT 11059200 (Crystal AT 11.0592MHZ)
 *
 * I DID USE ECLIPSE (BASIC VERSION WITH C) AND A VERY CONVENIANT PLUGINS "AVR Eclipse Plugin"
 * YOU SHOULD BE ABLE TO COMPILE WITHOUT ECLIPSE BY ENTERING "Release" FOLDER AND RUN "make all" IF YOU HAVE INSTALLED AVR env
 * THEN YOU SHOULD BURN THE RESULTING "multiplex.hex" FILE WITH AVRDUDE
 * ex with USBASP Programmer "avrdude -pm128 -cusbasp -Uflash:w:multiplex.hex:a"
 * You should also programe The fuse for the first use with the following param
 * ex with USBASP Programmer "avrdude "
 *
 * Author : Vincent de Boysson vinceofdrink@gmail.com
 * COMMERCIAL USE FORBIDDEN, PLEASE CONTACT ME !
 * TOUTE UTILISATION COMMERCIALE EST INTERDITE, CONTACTEZ MOI !
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************
 * External HARDWARE INVOLVED             *
 ******************************************/
// A TTL TO RS232 CONVERT (BASED ON MAX232) See on ebay
// A UBEC FROM HobbyKing for the VCC of Atmega (This is oversized choose whatever electonic that output a clean 5V like a dropout regulator) http://www.hobbyking.com/hobbyking/store/uh_viewItem.asp?idProduct=15212
// A Atmega128 Breakout Board in small factor size http://cgi.ebay.fr/ATMEGA128-16AU-CPU-PROTOTYPE-CPU-MODULE-MEGA128-/260720284318?pt=LH_DefaultDomain_0&hash=item3cb423d29e
// TTL 3.3V to 5V from sparkfun 	http://www.sparkfun.com/products/8745

/******************************************
 * Standard AVR LIB                       *
 ******************************************/
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>


/******************************************
 * Custom function library related to the project
 ******************************************/
//To Avoid function call that cost a lot of CPU cycle i did a lot of macro that look like function for basic recurrent operation
//Coding Disclaimer
//I could have use object programming rather than function and global variable
//The project is fairly simple and need to fullfill Realtime task based on hardware intteruption in good sync with the Frsky and Royal Evo
//I am shure there are elegant way to match those goals in OOP but i did the choice of simplicity.
//I also come straight from Arduino Programming so any advice on how to do this better is welcome
#include "macro_atmega.h"	//Macro of my own because i never remember the semantic of TIMER etc of AVR
#include "write_ppm.h"      //Provide all the function to write a PPM frame in asynchrone mode (using interrupt)
#include "serial.h"			//Provide basic access to UART0 and UART1 of Atmega 128
#include "royal_evo.h"		//Provide all the function to handle communication with Royal Evo Radio
#include "FrSky.h"			//Provide all the function to handle communicatio with FRSKY TX

void telemetry_with_debug(void);
/******************************************
 * OUTPUT USED ON ATMEGA128               *
 ******************************************/

// PB5 PPM OUTPUT 	-> CONNECT TO FRSKY INPUT PPM //  1K Ohm Resistor is problably welcome between AVR-FRSKY PPM CNX
// PE1 PDO/TXD0   	-> CONNECT TO RX OF ROYAL EVO USING A VOLTAGE DIVISOR LIKE 15K/10K to have a 3.3V TTL FROM THE AVR 5V TTL
// PE0 PDI/RXD0   	-> CONNECT TO TX OF ROYAL EVO USING A 10K RESITOR TO PROTECT HIGH CURRENT TO FLOW THERE

// PD3 INT3/TXD1(1) -> CONNECT TO THE TTL INPUT OF A RS232 TO TTL CONVERTER LIKE MAX232 TO THE INPUT PIN OF FRSKY RS232
// PD2 INT2/RXD1(1) -> CONNECT TO THE TTL OUTPUT OF RS232 TO TTL CONVERTER LIKE MAX232 TO THE OUPUT PIN OF FRSKY RS232
// YOU CAN USE ONLY THE FRSKY BASIC CONVERTER AND JUST WIRE THE RX TO HAVE ONLY THE READING OF TELEMETRY

// PA0 ->CONNECT TO THE VCC INPUT OF MAX232 TTL CONVERT FOR FRSKY (OPTIONAL YOU CAN PLUG DIRECTLY MAX232 VCC TO 5V)
// PA1 ->CONNECT TO A BUTTON THAT I PLACE ON THE OUTSIDE OF ROYAL EVO  CONNECT TO GND AND PA1 THE BUTTON WILL BE USE TO INTERACT WITH TELEMETRY OPTION AND CHANGE MODE AT BOOT TIME (PULL UP DONE WITH AVR ON THE PIN)

// PA4 BOOTLOADER MODE SET THE PIN

// PA5 & PA6 USED ACTUALY FOR DEBUG WITH LOGICAL ANALYSER SHOULD BE REMOVE WHEN CODE WILL REACH RELEASE STATUS
// PA7 ON BOARD LED -> USE FOR THE STATUT LED ON BOARD AND DEBUG PURPOSE(I USE A 1K Ohm Resistor)

// RESERVED FOR FUTURE USE TO DRIVE AN SD CARD IN FAT FORMAT (WITH AN EXTERNAL FAT LIBRARY)
// PB3	MISO (SPI Bus Master Input/Slave Output)
// PB2	MOSI (SPI Bus Master Output/Slave Input)
// PB1 SCK (SPI Bus Serial Clock)
// PB0 SS (SPI Slave Select input)


/******************************************
 * TIMER USAGE OF THE ATMEGA 128          *
 ******************************************/
// TIMER0 Is used to monitor Serial Input activity from royal evo, if we overflow we presume that Royal evo as send a entire frame
// TIMER1 Is used to forge the PPM signal using the Toogle property on the PB5 PORT
// TIMER0 Not used right now could be use to generate sound if don't find what we need with FRSKY and ROYAL EVO alarm.
// TIMER3 Will be used to listen and calculate value from an input PPM stream (For FPV HEAD TRACKER USAGE) that will be mixed with PPM output

/******************************************
 * USER DEFINED CONSTANT                  *
 ******************************************/
#define OPTION_WATCHDOG	0  //SET TO 1 IF WE WANT TO USE WATCHDOG TO RESET IF THE PROGRAM IS PRESUMED HANG
#define OPTION_BROWN_OUT 0 //SET TO 1 IF WE WANT TO CHECK BROWN_OUT OPTION TO DETECT VOLTAGE GARBAGE

#define MODE_FLY  		1	//NORMAL MODE
#define MODE_USB_ROYAL	2   //NOT AVAILABLE RIGHT NOW SHOULD BIND A USB-TO-TTL WITH ROYAL EVO PIN FOR UPGRADE OR BACKUP OF ROYAL DATA
#define MODE_USB_FRSKY	3   //NOT AVAILABLE RIGHT NOW SHOULD BIND THE SAME USB-TO-TTL WITH FRSKY MODULE FOR UPGRADE FIRMWARE FOR EXAMPLE
#define MODE_DEBUG		99	//SAME AS MODE FLY BUT TELEMETRY IS USED TO DISPLAY DEBUG PARAMETER
#define RSSI_EVO_ALARM	30	//Set the level below wich Royal evo will trigger RSSI Low Level Alarm if 0 no alarm at all (this is a bit redundant with FRSKY alarm dont know what to do with it)

/******************************************
 * Main Program                           *
 ******************************************/
//You should be able to do all your customisation in the main program
//i have try to expose all the high level event of the process here and all the telemetry action
int main(void)
{
	unsigned char mode;	//STORE WICH MODE WE RUN THE PROGRAM
	unsigned char standard_boot=TRUE; //TRUE for normal meaning (doing nego with royal and then listening to royal frame) FALSE (waiting about 500ms for nego and then go directly to listining evo frame)

	SET_PORT_AS_OUTPUT(A,7); //DEFINE LED PORT AS OUTPUT
	SET_PORT_AS_OUTPUT(A,0); //DRIVE VCC OF MAX232
	SET_PORT_HIGH(A,0); 	 //WE LIGHT UP MAX232 VCC

	SET_PORT_AS_OUTPUT(A,6);
	SET_PORT_AS_OUTPUT(A,5);
	SET_PORT_HIGH(A,5);
	SET_PORT_HIGH(A,6);

	//Tentative d'unfucking du port d'entre RX pour la royal evo !!
	SET_PORT_AS_INPUT(E,0);
	SET_PORT_LOW(E,0);


	SET_PORT_HIGH(A,7);		//LIGHT THE LED TO SAY HELLO :)


	//WE TRY TO DETECT WHAT KING OF POWER-ON WE ARE FACING
	//I plan to make a sonor alarm that would reflect BROWN-OUT and WATCHDOG event because
	//There are real threat for a cool and nice FLY :)
	#if OPTION_WATCHDOG==1
	//WATCHDOG ISSUE (NOT COOL MEANS A SOFTWARE BUG OR VOLTAGE GARBAGE AS STUCK
	//THE PROGRAM MORE THAT 120MS THATS ABOUT 4/5 PPM Cycle that have not been send (I reset Watchdog on every PPM CALL)
	if( READ_BIT(MCUCSR,WDRF))
	{
		// At least here i will try to bind again with royal evo even if i have to skip
		// The negotiation phase that maybe is al ready done and royal is problably sending me a Channel Position and dont wait for init sequence.
		// So if standard_boot is set a FALSE will try a few nego and if i dont get any reliable reply from evo i will go directly into reading channel position from evo.
		// I try to take care of those scenario to allow quick control recovery in case it append during a fly.

		standard_boot=FALSE;
		SB_LOW(MCUCSR,WDRF);	/We reset the WATCHDOG FLAG
	}
	#endif

	#if OPTION_BROWN_OUT==1
		//BROWN-OUT
		// Means we have poor power source for our avr and Brown-out as trigered a reset
		if( READ_BIT(MCUCSR,BORF))
		{

			standard_boot=FALSE;	//Same scenario as watchdog event
			SB_LOW(MCUCSR,WDRF);	//We reset the BROWN_OUT FLAG
		}
	#endif
	//Reset BY USER PRESSING RESET BUTTON
	if( READ_BIT(MCUCSR,EXTRF))
	{
		//Nothing special to do here
	}

	//Reset from Normal Power On
	if( READ_BIT(MCUCSR,PORF))
	{
		//nothing special to do here
	}


	//RIGHT NOW I ASSIGN BY DEFAULT THE MODE_FLY WICH IS THE NORMAL MODE
	//I NEED TO FIGURE OUT A WAY TO CHOOSE OTHER MODE AT BOOT TIME
	//the default mode would alway be MODE_FLY (Read Royal and Frsky And Writing PPM and tememetry)
	mode=MODE_FLY;

	//WE SUPPOSE TROUBLE ON BOOT  (WatchDog or Brown-out) FORCE the mode to MODE_FLY
	//AND TRY TO BEGIN AS FAST AS POSSIBLE THE PPM EMISSION
	if(!standard_boot)
		mode=MODE_FLY;

	mode=MODE_DEBUG;

	switch(mode)
	{

		case MODE_FLY:
		case MODE_DEBUG:
			Init_FrSky();				//Initialise Cnx with Frsky Module
			set_evo_rssi_alarm_level(RSSI_EVO_ALARM);	//Royal Evo RSSI Alarm Level See define for more info
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
					//We have at least 20 incoming bytes to decode if less we will wait for the next event
					if(serial0_input_writect>20)

					{

					//TOOGLE_PORT(A,7);//BLINK LED
					//DECODE PART AND PPM
					 TOOGLE_PORT(A,6);
					 decode_evo_data(); //Decode data from evo and fill the chanel value into ppm module
					 TOOGLE_PORT(A,6);
					 write_ppm();		//Write a PPM Signal (Asynchrone process)

					//We update the watchdog to prevent a reset
					//as we have declare a set 120MS Max the watch dog would trigger after  4/5 PPM Frame not being send after and reset the atmega128 (we handle such a restart)
					#if OPTION_WATCHDOG==1
					wdt_reset();
					#endif

					 Read_FrSky();//Read and decode Data receive from FRSKY
					 if(mode==MODE_DEBUG) //In mode debug we switch for a debug display using telemetry output
						telemetry_with_debug();
					 else
					 {
					//HANDLING TELEMETRY USER SERIAL DATA
					 /*
					  * NewUserDataFrSky() tell you if you have incoming byte from RX
					  * ReadUserDataFrSky() let you read one byte from RX (do not call if you don't have  NewUserDataFrSky()==TRUE
					  * ALl the byte Stuffing from FrSky have been handled you should see here just the byte you send from the RX
					  * (Be aware that on the RX size you should not execed a theorical 2400baud with the 9200baud line provided and probably less would be better so be
					  * wise when you send data from the RX)
					  * That being said you should be able to implement your own stuff instead of my protocol.
					 */

					 /* PROTOCOL HANDLING USER SERIAL DATA COMMING FROM FRSKY RX MODULE
					  * TO BE DONE
					 if(NewUserDataFrSky())
					 {
						 //HANDLE USER SERIAL STREAM FROM FRSKY (DATA SEND FROM RX)
						 unsigned char tmp;
						 tmp=ReadUserDataFrSky();
						 //DO NOTHING RIGHT NOW BUT I WILL DEFINE A PROTOCOL TO REMOTLY FROM RX ASSIGN DATA TO TELEMETRY CHANNEL
					 }
					  */

					 //set_royal_evo_rssi(get_FrSky_rssi_up_link());	//ASSIGN rssi that will allow alarm triggering for each telemetry frame even if there not related to RSSI


					 /**
					  * You can freely implement your own stuff here
					  * Just remember that we send 1 Telemetry per PPM Cycle "set_evo_telemetry" take care of that
					  * you just need to fill all telemetry from 0 to x if not for exemple if you just adress "set_evo_telemetry(0," and "set_evo_telemetry(2,"
					  * you will never see set_evo_telemetry(2 you should have use set_evo_telemetry(1, instead
					  *
					  * Usage set_evo_telemetry(0-11,UNIT_TYPE,signed int VALUE, Alarm)
					  * Unit type from evo :
					  *	UNIT_V,UNIT_A, UNIT_MS, UNIT_KMH, UNIT_RPM, UNIT_DEGC, UNIT_DEGF, UNIT_M, UNIT_FUEL, UNIT_LQI, UNIT_MAH, UNIT_ML, UNIT_D, UNIT_E, UNIT_F
					  */

						 set_evo_telemetry(0,UNIT_LQI,	get_FrSky_rssi_up_link()			,0);

						 set_evo_telemetry(1,UNIT_LQI,	get_FrSky_rssi_down_link()			,0);

											 //set_evo_telemetry(ct++,UNIT_V,	get_FrSky_sensor1()*3.3*10*4/256		,0);  //FILL THE VALUEs

						 set_evo_telemetry(2,UNIT_LQI,	get_royal_chanel(0)					,0); //DEBUG Stuff
					 }




					 _delay_ms(15);
					 // We wait for ppm frame to be done until that
					 // as we do not have nothing better to do we decode
					 // incoming information from FrSky()
					 // while (!is_ppm_active())Read_FrSky();


					 TOOGLE_PORT(A,6);
					 send_evo_telemetry();	//SEND THE TELEMETRY TO ROYAL EVO
					 TOOGLE_PORT(A,6);
					}
					//
						// We reset the serial buffer (we do not use in this project the circular buffer)
					 reset_trigger_for_next_data(); //Launch again the TIMER0 that monitor if we have data to handle from Royal Evo
					 //ooTIMER0_CT=0;
					 //ooTIMER0_OVERFLOW_ON;
					 //ooTIMER0_SCALE_8;
					 // OCR0=0;
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

extern unsigned int 	per_cycle_error;
extern unsigned char 	per_frame_error;
extern unsigned char 	frame_counter;
void telemetry_with_debug(void)
{

		unsigned char debug_type=2;
		switch (debug_type)
		{
			//Show the 9 computed chanel of royal evo
			case 0:
				set_evo_telemetry(0,UNIT_LQI,	get_royal_chanel(0)					,0);	//Chanel 1
				set_evo_telemetry(1,UNIT_LQI,	get_royal_chanel(1)					,0);	//Chanel 2
				set_evo_telemetry(2,UNIT_LQI,	get_royal_chanel(2)					,0);	//Chanel 3
				set_evo_telemetry(3,UNIT_LQI,	get_royal_chanel(3)					,0);	//Chanel 4
				set_evo_telemetry(4,UNIT_LQI,	get_royal_chanel(4)					,0);	//Chanel 5
				set_evo_telemetry(5,UNIT_LQI,	get_royal_chanel(5)					,0);	//Chanel 6
				set_evo_telemetry(6,UNIT_LQI,	get_royal_chanel(6)					,0);	//Chanel 7
				set_evo_telemetry(7,UNIT_LQI,	get_royal_chanel(7)					,0);	//Chanel 8
				set_evo_telemetry(8,UNIT_LQI,	get_royal_chanel(8)					,0);	//Chanel 9
			break;
			//Show 3 First values of buffer then royal chanel 1 computed value and then the size of the frame
			case 1:
				set_evo_telemetry(0,UNIT_LQI,	serial0_direct_buffer_read(0)		,0);	// 1 char of buffer
				set_evo_telemetry(1,UNIT_LQI,	serial0_direct_buffer_read(1)		,0);	// 2 char of buffer
				set_evo_telemetry(2,UNIT_LQI,	serial0_direct_buffer_read(2)		,0);	// 3 char of buffer
				set_evo_telemetry(3,UNIT_LQI,	get_royal_chanel(0)					,0);	// Chanel 1
				set_evo_telemetry(4,UNIT_LQI,	serial0_input_writect				,0);	// Royal Evo Buffer Size
			break;

			case 2:
							set_evo_telemetry(0,UNIT_LQI,	frame_counter						,0);
							if(per_frame_error!=0)
							{

							set_evo_telemetry(1,UNIT_LQI,	per_frame_error						,0);	// 2 char of buffer
							set_evo_telemetry(2,UNIT_LQI,	per_cycle_error						,0);	// 3 char of buffer
							set_evo_telemetry(3,UNIT_LQI,	serial0_direct_buffer_read(0)		,0);	// 2 char of buffer
							set_evo_telemetry(4,UNIT_LQI,	serial0_direct_buffer_read(1)		,0);	// 2 char of buffer
							set_evo_telemetry(5,UNIT_LQI,	serial0_direct_buffer_read(2)		,0);	// 3 char of buffer
							}
							else
							{
								set_evo_telemetry(6,UNIT_LQI,	serial0_direct_buffer_read(0)		,0);	// 2 char of buffer
								set_evo_telemetry(7,UNIT_LQI,	serial0_direct_buffer_read(1)		,0);	// 2 char of buffer
								set_evo_telemetry(8,UNIT_LQI,	serial0_direct_buffer_read(2)		,0);	// 3 char of buffer
							}

			break;

		}

}

