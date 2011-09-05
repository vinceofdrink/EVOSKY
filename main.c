/*
 * PROJECT INTEND TO WORK ON ATMEGA128 WITH F_CPU AT 11059200 (Crystal AT 11.0592MHZ) AND ROYAL EVO 7,9,12 FIRMWARE 1.41
 * I DID NOT TEST FOR REAL ROYAL 7 AND ROYAL 12
 *
 * BASIC FUNCTION ARE
 * DECODE ROYAL_EVO DATA AND GENERATE ACCORDINGLY A  PPM OUTPUT FOR FRSKY MODULE
 * DECODE FRSKY TELEMETRY DATA AND TRANSLATE THEM TO ROYAL EVO TELEMERTRY DISPLAY
 * DECODE A PPM INPUT FROM A FPV HEAD TRACKER AND ASSIGN THEN TO ANY OF THE TRANSMITED CHANEL
 *
 * THE TELEMETRY SHOULD BE EASYLY MODIFY TO MEET YOUR NEED
 *
 * I USE ECLIPSE (BASIC VERSION WITH C) AND A VERY CONVENIANT PLUGINS "AVR Eclipse Plugin" TO EDIT THIS PROJECT
 * YOU SHOULD BE ABLE TO COMPILE WITHOUT ECLIPSE BY ENTERING "Release" FOLDER AND RUN "make all" IF YOU HAVE INSTALLED AVR ENVIRONEMENT
 * THEN YOU SHOULD BURN THE RESULTING "multiplex.hex" FILE WITH AVRDUDE OR USING THE AVRUBD UPLOADER IF YOU HAVE
 * CHOOSE FIRST TO BURN ATMEGA128 WITH THE AVR°UNIVERSAL_BOOT LOADER PROVIDE WITH THE EVOSKY PROJECT
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
 * FuseBit  an programming  			  *
 ******************************************/
/*

avrdude	-pm128 -cusbasp -u
			-Uflash:w:EVOSKY.hex:a
			-Ulfuse:w:0xde:m
			-Uhfuse:w:0xde:m
			-Uefuse:w:0xff:m

 */
/******************************************
 * External HARDWARE INVOLVED             *
 ******************************************/
// A TTL TO RS232 CONVERTER (BASED ON MAX232) See on ebay
// A UBEC FROM HobbyKing for the VCC of Atmega (This is oversized choose whatever electonic that output a clean 5V like a dropout regulator) http://www.hobbyking.com/hobbyking/store/uh_viewItem.asp?idProduct=15212
// A Atmega128 Breakout Board in small factor size http://cgi.ebay.fr/ATMEGA128-16AU-CPU-PROTOTYPE-CPU-MODULE-MEGA128-/260720284318?pt=LH_DefaultDomain_0&hash=item3cb423d29e


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
#include <avr/eeprom.h>

/******************************************
 * Custom function library related to the project
 ******************************************/
//To Avoid function call that cost a lot of CPU cycle i did a lot of macro that look like function for basic recurrent operation
//Coding Disclaimer
//I could have use object programming rather than function and global variable
//The project is fairly simple and need to fullfill Realtime task based on hardware intteruption in good sync with the Frsky and Royal Evo
//I am shure there are elegant way to match those goals in OOP but i did the choice of simplicity.
//I also come straight from Arduino Programming so any advice on how to do this better is welcome

//All the setting you could need to edit are grouped into settings.h
#include "settings.h"

#include "macro_atmega.h"	//Macro of my own because i never remember the semantic of TIMER etc of AVR
#include "write_ppm.h"      //Provide all the function to write a PPM frame in asynchrone mode (using interrupt)
#include "read_ppm.h"      //Provide all the function to listen to a PPM stream in asynchrone mode (using interrupt)
#include "serial.h"			//Provide basic access to UART0 and UART1 of Atmega 128
#include "royal_evo.h"		//Provide all the function to handle communication with Royal Evo Radio
#include "FrSky.h"			//Provide all the function to handle communicatio with FRSKY TX
#include "button.h"			//Provide all the detection of button interaction with user
void telemetry_debug(unsigned char debug_type);


/******************************************
 * PIN USED ON ATMEGA128               	  *
******************************************/

// PB5 PPM OUTPUT 	-> CONNECT TO FRSKY INPUT PPM //  1K Ohm Resistor is problably welcome between AVR-FRSKY PPM CNX


// PE1 OR PD3 (Depending of UARTCHOICE )PDO/TXD0   	-> CONNECT TO RX OF ROYAL EVO USING A VOLTAGE DIVISOR LIKE 15K/10K to have a 3.3V TTL FROM THE AVR 5V TTL
// PE0 OR PD2 (Depending of UARTCHOICE ) PDI/RXD0   	-> CONNECT DIRECTLY TO TX OF ROYAL EVO
// PE7 PPM INPUT 	-> CONNECT TO YOUR FPV HEAD TRACKER (IN DEVELOPEMENT)

// PD3 INT3/TXD1(1) -> CONNECT TO THE TTL INPUT OF A RS232 TO TTL CONVERTER LIKE MAX232 TO THE INPUT PIN OF FRSKY RS232
// PD2 INT2/RXD1(1) -> CONNECT TO THE TTL OUTPUT OF RS232 TO TTL CONVERTER LIKE MAX232 TO THE OUPUT PIN OF FRSKY RS232
// YOU CAN USE ONLY THE FRSKY BASIC CONVERTER AND JUST WIRE THE RX TO HAVE ONLY THE READING OF TELEMETRY

// PA0 ->CONNECT TO THE VCC INPUT OF MAX232 TTL CONVERT FOR FRSKY (OPTIONAL YOU CAN PLUG DIRECTLY MAX232 VCC TO 5V)
// PE4 ->CONNECT TO A BUTTON PLACED ON THE OUTSIDE OF ROYAL EVO  CONNECT TO GND AND PE4 THE BUTTON WILL BE USE TO INTERACT WITH TELEMETRY OPTION AND CHANGE MODE AT BOOT TIME (PULL UP DONE WITH AVR ON THE PIN)
// PE5 ->CONNECT TO A BUTTON PLACED ON THE OUTSIDE OF ROYAL EVO  CONNECT TO GND AND PE5 THE BUTTON WILL ON START_USED TO DETECT IF WE WANT ENTER BOOTLOADER MODE TO UPDATE EVOSKY FIRMWARE

// BOOTLOADER PART IS STANDBY RIGHT NOW BECAUSE I DID NOT ACHIEVE TO INSTALL A WORKING BOOTLOADER ON MY ATEMEGA128
// IF SOMEONE COULD HELP IT WILL BE WELCOME.
// PA4 BOOTLOADER MODE SET THE PIN TO THE GROUND WHEN YOU WANT TO GO IN BOOTLOADER MODE IN ORDER TO UPDATE THE FIRMWARE.
// BOOATLOADER WILL USE THE SAME UART AS FRSKY (PD3 TX ,PD2 RX) THAT THE REASON WHY I USE A PIN TO ACTIVATE

// THIS PIN ARE OPTIONAL AND USED IN MODE "MODE_USB_ROYAL"
// PA5 SHOULD BE CONNECTED TO RX PIN OF ROYAL EVO PROGRAMING PORT
// PA6 SHOULD BE CONNECTED TO TX PIN OF ROYAL EVO PROGRAMING PORT

// BUT PA5 A
// PA7 ON BOARD LED -> USE FOR THE STATUT LED ON BOARD AND DEBUG PURPOSE(I USE A 1K Ohm Resistor)

// RESERVED FOR FUTURE USE TO DRIVE AN SD CARD TO STORE TELEMETRY HISTORY IN FAT FORMAT (WITH AN EXTERNAL FAT LIBRARY)
// WARNING DONT GET FOOLED WITH MISO AND MOSI LABEL USED FOR PROGRAMMING THE ATMEGA128 AS THERE ARE CONNECT TO PE0 PE1
// WILL WORK ON THIS FOR WINTER TIME DO NOT EXPECT THIS TO BE SOON AVAILABLE
// PB3	MISO (SPI Bus Master Input/Slave Output)
// PB2	MOSI (SPI Bus Master Output/Slave Input)
// PB1 	SCK (SPI Bus Serial Clock)
// PB0 	SS (SPI Slave Select input)


/******************************************
 * TIMER USAGE OF THE ATMEGA 128          *
 ******************************************/
// TIMER0 Is used to monitor Serial Input activity from royal evo, if we overflow we presume that Royal evo as send a entire frame
// TIMER1 (16bit) Is used to build the PPM signal using the Toogle property on the PB5 PORT
// TIMER2 Not used right now could be use to generate sound if we don't find what we need with FRSKY and ROYAL EVO alarm.
// TIMER3 Will be used to listen and calculate value from an input PPM stream on PE7 using external event timestamp ICR3 (For FPV HEAD TRACKER USAGE) that will be mixed with PPM output

/* MEMO  EXTERNAL INTERRUPT
 *
 //USED
 INTO PD0 //
 INT1 PD1
 INT2 PD2
 INT3 PD3
 INT7 PE7 //PPM INPUT FOR HEADTRACKER

 INT5 PE5 //BUTTON
 INT4 PE4 //BUTTON

 //FREE
 INT6 PE6


 */


/*****************************************
 * THE DIFFERENT OPERATING MODE OF THE EVOSKY (THERE ARE SELECTED AT BOOT TIME BY PRESSING THE EXTERNAL BUTTON) DO NOT EDIT CODE VALUE THERE NO NEED FOR IT
 *****************************************/
#define MODE_FLY  			0	//NORMAL MODE
#define MODE_FLY_FPV		1	//NORMAL MODE + READING OF INPUT PIN PE7 FOR A PPM STREAM FROM FPV HEAD TRACKER
#define MODE_FLY_FPV_MIX	2	//NORMAL MODE + READING OF INPUT PIN PE7 FOR A PPM STREAM FROM FPV HEAD TRACKER BUT OUTPUT WILL BE MIXED ON FPV_CHANEL_1_OUT NEED DECODER ON THE OTHER SIDE

#define MODE_USB_ROYAL		3   //NOT TESTED BUT SHOULD WORK
#define MODE_USB_FRSKY		4   //NOT AVAILABLE RIGHT NOW SHOULD BIND THE SAME USB-TO-TTL WITH FRSKY MODULE FOR UPGRADE FIRMWARE FOR EXAMPLE

#define TELE_DISPLAY_STD		0
#define TELE_DISPLAY_CHANEL		1
#define TELE_DISPLAY_PPM_OUT	2
#define TELE_DISPLAY_PPM_IN		3
#define TELE_DISPLAY_END		4
/******************************************
 * Main Program                           *
 ******************************************/
//You should be able to do all your customisation in the main program
//i have try to expose all the high level event of the process here and all the telemetry action
int main(void)
{
	unsigned char mode;					//STORE WICH MODE WE RUN THE EVOSKY
	unsigned char standard_boot=TRUE; 	//TRUE for normal meaning (doing nego with royal and then listening to royal frame) FALSE (waiting about 500ms for nego and then go directly to listining evo frame)
	unsigned char display_mode=TELE_DISPLAY_STD;
	SET_PORT_AS_OUTPUT(A,7); 			//DEFINE LED PORT AS OUTPUT
	SET_PORT_HIGH(A,7);					//LIGHT THE LED TO SAY HELLO :)

	SET_PORT_LOW(A,0); 	 				//WE LIGHT UP MAX232 VCC LATER ON
	SET_PORT_AS_OUTPUT(A,0); 			//DRIVE VCC OF MAX232




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

		standard_boot=FALSE;	// We wait only 500ms for royal evo nego and then go to listining for frame position (the royal evo as probably not been reset)
		SB_LOW(MCUCSR,WDRF);	// We reset the WATCHDOG FLAG
	}
	#endif

	#if OPTION_BROWN_OUT==1
		//BROWN-OUT
		// Means we have poor power source for our avr and Brown-out as trigered a reset
		if( READ_BIT(MCUCSR,BORF))
		{

			standard_boot=FALSE;	//Same scenario as watchdog event but we should warn the user about this event (sound maybe)
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
	if(SET_PORT_AS_OUTPUT(A,0))


	//Mode will be swichable with button later
	mode=MODE_FLY; //MODE_USB_ROYAL
	standard_boot=FALSE;
	switch(mode)
	{

		case MODE_FLY:
		case MODE_FLY_FPV:
		case MODE_FLY_FPV_MIX:




			// eeprom_write_byte ((uint8_t*)0, sizeof(royal_memo));
			SET_PORT_HIGH(A,0); 	 	//WE LIGHT UP MAX232 VCC
			Init_FrSky();				//Initialise Cnx with Frsky Module
			set_evo_rssi_alarm_level(RSSI_EVO_ALARM);	//Royal Evo RSSI Alarm Level See define for more info
			init_royal(standard_boot);	//Initialise Cnx with Royal Evo (standard boot means we will wait for real negotiation with royal evo otherwise if we cannot akwnoledge Royal evo we skip try several nego and then start directly to listening to channel position stream

			init_evo_model_storage(0);
			royal_memo.fpv_chanel_1_offset++;
			store_evo_model(0);
			init_ppm();					//Initialise PPM Writer
			init_read_ppm(1);
			set_evo_telemetry(0,UNIT_LQI,	0				,0);
			init_button();


			//We activate the watchdog that will trigger a reset if wdt_reset() is not call every 120MS
			#if OPTION_WATCHDOG==1
				wdt_enable(WDTO_120MS);
			#endif
			if (mode==MODE_FLY_FPV || mode==MODE_FLY_FPV_MIX)
				init_read_ppm(FPV_PPM_TYPE);
			while(TRUE)
			{

				if(royal_trame_ok())	//Test if we have a valid data from EVO
				{

					if(evo_uart_input_writect==0)

					{

					TOOGLE_PORT(A,7);//BLINK LED

					decode_evo_data(); //Decode data from evo and fill the chanel value into ppm module
					write_ppm();		//Write a PPM Signal (Asynchrone process)

					//We update the watchdog to prevent a reset
					//as we have declare a set 120MS Max the watch dog would trigger after  4/5 PPM Frame not being send after and reset the atmega128 (we handle such a restart)
					#if OPTION_WATCHDOG==1
					wdt_reset();
					#endif

					 Read_FrSky();//Read and decode Data receive from FRSKY
					 compute_ppm_input();//Decode the last PPM captured data

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
					#if RSSI_EVO_ALARM!=0
						 set_royal_evo_rssi(get_FrSky_rssi_up_link());	//ASSIGN rssi that will allow alarm triggering for each telemetry frame even if there not related to RSSI
					#endif

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


						 /*


						 set_evo_telemetry(4,UNIT_LQI,	get_royal_chanel(4)					,0);	//Chanel 5
						 				set_evo_telemetry(5,UNIT_LQI,	get_royal_chanel(5)					,0);	//Chanel 6
						 				set_evo_telemetry(6,UNIT_LQI,	get_royal_chanel(6)					,0);	//Chanel 7
						 				set_evo_telemetry(7,UNIT_LQI,	get_royal_chanel(7)					,0);	//Chanel 8
						 				set_evo_telemetry(8,UNIT_LQI,	get_royal_chanel(8)					,0);	//Chanel 9
						*/
						 //Handle user interaction
						 switch(get_button_state())
						 {
							 case BUTTON_NO_EVENT:

							 break;
							 case BUTTON1_SINGLE_CLICK:
								 //if we are in not in EVO_DISPLAY_NORMAL then resume to normal display
								 if(get_evo_display_mode()!=EVO_DISPLAY_NORMAL)
									 set_evo_display_mode(EVO_DISPLAY_NORMAL);
								 else//make a down cursor move.
									 evo_cursor_down();
							 break;
							 case BUTTON1_DOUBLE_CLICK:

							 break;

							 case BUTTON1_LONG_CLICK:
								// if(evo_cursor_active())

								// else
								 set_evo_display_mode(EVO_DISPLAY_LOW);
							 break;

							 case BUTTON2_SINGLE_CLICK:
								 if(get_evo_display_mode()!=EVO_DISPLAY_NORMAL)
										set_evo_display_mode(EVO_DISPLAY_NORMAL);
									else//make a down cursor move.
										 evo_cursor_up();

							 break;

							 case BUTTON2_DOUBLE_CLICK:

							 break;

							 case BUTTON2_LONG_CLICK:
								 set_evo_display_mode(EVO_DISPLAY_HIGH);

							 break;

							 case BUTTON_BOTH_SINGLE_CLICK:
								 display_mode++;
								 if(display_mode==TELE_DISPLAY_END)
									 display_mode=TELE_DISPLAY_STD;
							 break;
						 }
						 static unsigned int compute_free_cycle=0;
						 if(display_mode==TELE_DISPLAY_STD)
						 {
							 //Tester UNIT_D
							 set_evo_telemetry(0,UNIT_LQI,	get_FrSky_rssi_up_link()				,0);

							 set_evo_telemetry(1,UNIT_LQI,	royal_memo.fpv_chanel_1_offset			,0);
							 //set_evo_telemetry(1,UNIT_LQI,	get_FrSky_rssi_down_link()				,0);

							 set_evo_telemetry(2,UNIT_V,	(get_FrSky_sensor1()*3.3*10*4/256)	+1	,0);

							 set_evo_telemetry(3,UNIT_V,	get_FrSky_sensor2()*3.3*10*4/256		,0);

							 set_evo_telemetry(4,UNIT_LQI,	compute_free_cycle/100		,0);
						 }
						 else
							 telemetry_debug(display_mode);
					//_delay_ms(14);
					 // We wait for ppm frame to be done until that
						 compute_free_cycle=0;
					 while (is_ppm_active())
					 {

						 //Read_FrSky();
						 compute_free_cycle++;
					 }



					 send_evo_telemetry();	//SEND THE TELEMETRY TO ROYAL EVO

					}
					//
						// We reset the serial buffer (we do not use in this project the circular buffer)
					 reset_trigger_for_next_data(); //Launch again the TIMER0 that monitor if we have data to handle from Royal Evo

				}
			}
		break;

		// MODE_USB_ROYAL AND MODE_USB_FRSKY
		// SUPPOSE THAT YOU HAVE WIRE A USB TTL CONNECTOR TO THE PD2 and PD3 PIN AND IDEALY THE USB CONNECTOR POP OUT OF THE ROYAL EVO BOX
		// THAT THIS USB TTL HAVE HIGH RESISTANCE ON THOSE PIN WHEN NOT POWERED
		// THAT THIS USB TTL PROVIDE A VCC TO 5V THAT WILL ADVERTISE EVOSKY THAT WE HAVE AN COMPUTER CONNECT TO USB
		// THAT THE MAX 232 AS HIGH RESISTANCE ON IS TX an RX TTL PIN
		// IF ALL THOSE CONDITION ARE OK WE WILL HAVE THE POSIBILITY FROM USB PORT TO
		// BURN NEW FIRMWARE FOR EVOSKY WITHOUT THE NEED OF PROGRAM
		// UPDATE FRSKY FIRMWARE WITH FRSKY PC SOFTWARE
		// UPDATE ROYAL AND MANAGE ROYAL EVO FIRMWARE AND SETTING WITH MULTIPLEX SOFTWARE
		// IN FUTURE WE COULD ALSO OUTPUT STICK POSITION FOR SIMULATOR
		case MODE_USB_ROYAL:
			//WE WILL BIT BANG THE STATE OF PD2 TO ROYAL SERIAL INPUT PIN
			//WE WILL BIT BANG THE STATE OF ROYAL SERIAL OUTPUT PIN TO PD3
			//AS WE SHARE THE SAME PORT THAT IS USED BY FRSKY THE MAX232 SHOULD NOT BE POWERED  (SEE PA0 PIN) SO NO ARETFACT WITH FRSKY MODULE
			//THIS IS A BIT A BRUTAL METHOD BUT IT SHOULD WORK WE HAVE NOTHING ELSE TO DO AND NO INTERRUPTION IN BACKGROUND
			//WAIT TO WIRE A TTL TO USB CONVERTER TO EVOSKY BOARD TO TEST

			SET_PORT_LOW(A,3);
			SET_PORT_AS_INPUT(A,3);

			SET_PORT_LOW(A,4);
			SET_PORT_AS_OUTPUT(A,4);

			SET_PORT_LOW(A,5);
			SET_PORT_AS_OUTPUT(A,5);

			SET_PORT_LOW(A,6);
			SET_PORT_AS_INPUT(A,6);


			while(1)
			{
				//COPY STATE OF USB OUTPUT TO ROYAL INPUT
				if(READ_PORT_INPUT(A,3))
					SET_PORT_HIGH(A,5);
				else
					SET_PORT_LOW(A,5);

				//COPY STATE OF ROYAL OUTPUT TO USB INPUT
				if(READ_PORT_INPUT(A,6))
					SET_PORT_HIGH(A,4);
				else
					SET_PORT_LOW(A,4);
			}




		break;

		case MODE_USB_FRSKY:
			//PRETTY SIMPLE  WE JUST NEED TO KEEP PD2 & PD3 TO INPUT MODE WITH NO PULL UP SO THERE WILL NOT INTERACT
			//WITH COMMUNICATION BETWEEN THE USB TTL AND THE FRSKY MODULE ON PD3 AND PD2
			//WE JUST NEED TO LIGHT UP THE MAX232
			SET_PORT_AS_INPUT(D,2);
			SET_PORT_LOW(D,2);

			SET_PORT_AS_INPUT(D,3);
			SET_PORT_LOW(D,3);
			SET_PORT_HIGH(A,0); 	 			//WE LIGHT UP MAX232 VCC
			while(TRUE)
			{
				//nop;
			}
		break;

	}


return 1;
}


#if F_CPU == 16000000
  #define CT_TO_US                        2                  // 2 for prescaler 8
#elif F_CPU == 18432000
  #define CT_TO_US                        2.304              //  2.304 for prescaler 8
#elif F_CPU == 8000000
  #define CT_TO_US                        1                  //  1 for prescaler 8
#elif F_CPU == 11059200
  #define CT_TO_US                        1.3824
#endif


extern unsigned int 	per_cycle_error;
extern unsigned char 	per_frame_error;
extern unsigned char 	frame_counter;
extern volatile	unsigned char 	g_read_ppm_ct;
void telemetry_debug(unsigned char debug_type)
{



		switch (debug_type)
		{
			//Show the 9 computed chanel of royal evo
			case TELE_DISPLAY_CHANEL:
				set_evo_telemetry_debug(0,UNIT_LQI,	per_cycle_error						,0);	//Chanel 1
				set_evo_telemetry_debug(1,UNIT_LQI,	get_royal_chanel(0)					,0);	//Chanel 2
				set_evo_telemetry_debug(2,UNIT_LQI,	get_royal_chanel(1)					,0);	//Chanel 3
				set_evo_telemetry_debug(3,UNIT_LQI,	get_royal_chanel(2)					,0);	//Chanel 4
				set_evo_telemetry_debug(4,UNIT_LQI,	get_royal_chanel(3)					,0);	//Chanel 5
				set_evo_telemetry_debug(5,UNIT_LQI,	get_royal_chanel(4)					,0);	//Chanel 6
				set_evo_telemetry_debug(6,UNIT_LQI,	get_royal_chanel(5)					,0);	//Chanel 7
				set_evo_telemetry_debug(7,UNIT_LQI,	get_royal_chanel(6)					,0);	//Chanel 8
				set_evo_telemetry_debug(8,UNIT_LQI,	get_royal_chanel(7)					,0);	//Chanel 9
				set_evo_telemetry_debug(9,UNIT_LQI,	get_royal_chanel(8)					,0);	//Chanel 9
			break;
			//Show the -1 to advertize witch debug mode we are and display after the us offset of each PPM chanel
			case TELE_DISPLAY_PPM_OUT:
				set_evo_telemetry_debug(0,UNIT_LQI,	-1									,0);	//Chanel 1
				set_evo_telemetry_debug(1,UNIT_LQI,	get_ppm1_chanel(0)/CT_TO_US			,0);	//Chanel 2
				set_evo_telemetry_debug(2,UNIT_LQI,	get_ppm1_chanel(1)/CT_TO_US			,0);	//Chanel 3
				set_evo_telemetry_debug(3,UNIT_LQI,	get_ppm1_chanel(2)/CT_TO_US			,0);	//Chanel 4
				set_evo_telemetry_debug(4,UNIT_LQI,	get_ppm1_chanel(3)/CT_TO_US			,0);	//Chanel 5
				set_evo_telemetry_debug(5,UNIT_LQI,	get_ppm1_chanel(4)/CT_TO_US			,0);	//Chanel 6
				set_evo_telemetry_debug(6,UNIT_LQI,	get_ppm1_chanel(5)/CT_TO_US			,0);	//Chanel 7
				set_evo_telemetry_debug(7,UNIT_LQI,	get_ppm1_chanel(6)/CT_TO_US			,0);	//Chanel 8
				set_evo_telemetry_debug(8,UNIT_LQI,	get_ppm1_chanel(7)/CT_TO_US			,0);	//Chanel 9
			break;

			//Show the -2 to advertize witch debug mode we are and display after the us offset of each  INPUT reading from HEADTRACKER PPM chanel
			case TELE_DISPLAY_PPM_IN:
				set_evo_telemetry_debug(0,UNIT_LQI,	-2									,0);
				set_evo_telemetry_debug(1,UNIT_LQI,	g_read_ppm_ct						,0);
				set_evo_telemetry_debug(2,UNIT_LQI,	read_ppm_chanel(0)/CT_TO_US			,0);	//Chanel 2
				set_evo_telemetry_debug(3,UNIT_LQI,	read_ppm_chanel(1)/CT_TO_US			,0);	//Chanel 3
				set_evo_telemetry_debug(4,UNIT_LQI,	read_ppm_chanel(2)/CT_TO_US			,0);	//Chanel 4
				set_evo_telemetry_debug(5,UNIT_LQI,	read_ppm_chanel(3)/CT_TO_US			,0);	//Chanel 5
				set_evo_telemetry_debug(6,UNIT_LQI,	read_ppm_chanel(4)/CT_TO_US			,0);	//Chanel 6
				set_evo_telemetry_debug(7,UNIT_LQI,	read_ppm_chanel(5)/CT_TO_US			,0);	//Chanel 7
				set_evo_telemetry_debug(8,UNIT_LQI,	read_ppm_chanel(6)/CT_TO_US			,0);	//Chanel 8
				set_evo_telemetry_debug(9,UNIT_LQI,	read_ppm_chanel(7)/CT_TO_US			,0);	//Chanel 9
			break;

		}

}

