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
#include "read_ppm.h"      //Provide all the function to listen to a PPM stream in asynchrone mode (using interrupt)
#include "serial.h"			//Provide basic access to UART0 and UART1 of Atmega 128
#include "royal_evo.h"		//Provide all the function to handle communication with Royal Evo Radio
#include "FrSky.h"			//Provide all the function to handle communicatio with FRSKY TX

void telemetry_with_debug(void);

//SOME Global to store button state
unsigned char evo_bt1=0;
unsigned char evo_bt2=0;
unsigned char evo_bt1_timestamp=0;
unsigned char evo_bt2_timestamp=0;


// Check for single or double click action.
#define button_doubleclick_timming 	2 //1 =+- 20ms (PPM FRAME)

//Monitor click action

#define evosky_button1_click()	(evo_bt1!=0 && evo_bt1_timestamp>=button_doubleclick_timming)
#define evosky_button2_click()	(evo_bt2!=0 && evo_bt2_timestamp>=button_doubleclick_timming)

#define evosky_button1_double_click()		(evo_bt1>2)
#define evosky_button2_double_click()  		(evo_bt2>2)

#define evosky_button1_long_click()			(evo_bt1==1)
#define evosky_button2_long_click() 		(evo_bt2==1)

#define evosky_button_reset()				evo_bt1=evo_bt2=0;
/******************************************
 * PIN USED ON ATMEGA128               *
 ******************************************/

// PB5 PPM OUTPUT 	-> CONNECT TO FRSKY INPUT PPM //  1K Ohm Resistor is problably welcome between AVR-FRSKY PPM CNX
// PE1 PDO/TXD0   	-> CONNECT TO RX OF ROYAL EVO USING A VOLTAGE DIVISOR LIKE 15K/10K to have a 3.3V TTL FROM THE AVR 5V TTL
// PE0 PDI/RXD0   	-> CONNECT DIRECTLY TO TX OF ROYAL EVO
// PE7 PPM INPUT 	-> CONNECT TO YOUR FPV HEAD TRACKER (IN DEVELOPEMENT)

// PD3 INT3/TXD1(1) -> CONNECT TO THE TTL INPUT OF A RS232 TO TTL CONVERTER LIKE MAX232 TO THE INPUT PIN OF FRSKY RS232
// PD2 INT2/RXD1(1) -> CONNECT TO THE TTL OUTPUT OF RS232 TO TTL CONVERTER LIKE MAX232 TO THE OUPUT PIN OF FRSKY RS232
// YOU CAN USE ONLY THE FRSKY BASIC CONVERTER AND JUST WIRE THE RX TO HAVE ONLY THE READING OF TELEMETRY

// PA0 ->CONNECT TO THE VCC INPUT OF MAX232 TTL CONVERT FOR FRSKY (OPTIONAL YOU CAN PLUG DIRECTLY MAX232 VCC TO 5V)
// PA1 ->CONNECT TO A BUTTON PLACED ON THE OUTSIDE OF ROYAL EVO  CONNECT TO GND AND PA1 THE BUTTON WILL BE USE TO INTERACT WITH TELEMETRY OPTION AND CHANGE MODE AT BOOT TIME (PULL UP DONE WITH AVR ON THE PIN)
// PA2 ->CONNECT TO A BUTTON PLACED ON THE OUTSIDE OF ROYAL EVO  CONNECT TO GND AND PA2 THE BUTTON WILL ON START_USED TO DETECT IF WE WANT ENTER BOOTLOADER MODE TO UPDATE EVOSKY FIRMWARE

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
 INTO PD0
 INT1 PD1
 INT2 PD2
 INT3 PD3
 INT7 PE7

 //FREE
 INT6 PE6
 INT5 PE5
 INT4 PE4

 */
/******************************************
 * USER DEFINED OPTION                    *
 ******************************************/
#define OPTION_WATCHDOG		0  		//SET TO 1 IF WE WANT TO USE WATCHDOG TO RESET IF THE PROGRAM IS PRESUMED HANG
#define OPTION_BROWN_OUT 	0 		//SET TO 1 IF WE WANT TO CHECK BROWN_OUT OPTION TO DETECT VOLTAGE GARBAGE
#define RSSI_EVO_ALARM		30		//Set the level below wich Royal evo will trigger RSSI Low Level Alarm if 0 no alarm at all (this is a bit redundant with FRSKY alarm dont know what to do with it)

// ALL THE FPV_CHANEL MIXING IS DESCRIBE BUT NOT DEVELOPED RIGHT NOW
#define FPV_PPM_TYPE		1	// 1 for positive PPM 0 for negative PPM
#define FPV_CHANEL_1_IN		1	//WICH CHANEL WE SHOULD READ FROM HEAD TRACKER PPM FOR OUR CHANEL1
#define FPV_CHANEL_2_IN		2	//WICH CHANEL WE SHOULD READ FROM HEAD TRACKER PPM FOR OUR CHANEL2
#define FPV_CHANEL_1_OUT	7	//ON WICH FRSKY OUTPUT WE WISH TO OUTPUT OUR FPV_CHANEL1 (IN MIX MODE ONLY CHANEL_1 WILL BE USED
#define FPV_CHANEL_2_OUT	8	//ON WICH FRSKY OUTPUT WE WISH TO OUTPUT OUR FPV_CHANEL2


// THE PROCESS OF CHANNEL MIXING IS TO DIVIDE BY 2 THE RESOLUTION AND ASSIGN CHANEL 1 INTO (-100% -> 0%) AND CHANEL 2 (0%->+100%)
// WE CAN CONFIGURE A MIDDLE MARGIN TO AVOID CONFUSION BETWEEN C1 AND C2 SO CHANNEL 1 AND CHANNEL 2 WILL NEVER BE SET INTO A DEAD BAND DEFINED BELOW IN uS
// THE FPV_CHANEL_MIXING_MARGIN IS AT THE CENTER OF THE SIGNAL
// OF COURSE CHANEL MIXING NEED AN ARDUINO  CONNECTED TO THE FRSKY RX MODULE TO THE FPV_CHANEL_1_OUT CHANEL PIN TO SPLIT AGAIN THE CHANNEL AND SHOULD SHARE THE SAME MARGIN
#define FPV_CHANEL_MIXING_MARGIN	50		//in uS   ( 1sec /1000 = Milliseconde /1000 = uS)

/*****************************************
 * THE DIFFERENT OPERATING MODE OF THE EVOSKY (THERE ARE SELECTED AT BOOT TIME BY PRESSING THE EXTERNAL BUTTON) DO NOT EDIT CODE VALUE THERE NO NEED FOR IT
 *****************************************/
#define MODE_FLY  			0	//NORMAL MODE
#define MODE_FLY_FPV		1	//NORMAL MODE + READING OF INPUT PIN PE7 FOR A PPM STREAM FROM FPV HEAD TRACKER
#define MODE_FLY_FPV_MIX	2	//NORMAL MODE + READING OF INPUT PIN PE7 FOR A PPM STREAM FROM FPV HEAD TRACKER BUT OUTPUT WILL BE MIXED ON FPV_CHANEL_1_OUT NEED DECODER ON THE OTHER SIDE

#define MODE_USB_ROYAL		3   //NOT TESTED BUT SHOULD WORK
#define MODE_USB_FRSKY		4   //NOT AVAILABLE RIGHT NOW SHOULD BIND THE SAME USB-TO-TTL WITH FRSKY MODULE FOR UPGRADE FIRMWARE FOR EXAMPLE

/******************************************
 * Main Program                           *
 ******************************************/
//You should be able to do all your customisation in the main program
//i have try to expose all the high level event of the process here and all the telemetry action
int main(void)
{
	unsigned char mode;					//STORE WICH MODE WE RUN THE EVOSKY
	unsigned char standard_boot=TRUE; 	//TRUE for normal meaning (doing nego with royal and then listening to royal frame) FALSE (waiting about 500ms for nego and then go directly to listining evo frame)







	SET_PORT_AS_OUTPUT(A,7); 			//DEFINE LED PORT AS OUTPUT

	SET_PORT_LOW(A,0); 	 				//WE LIGHT UP MAX232 VCC LATER ON
	SET_PORT_AS_OUTPUT(A,0); 			//DRIVE VCC OF MAX232


	//BUTTON INTERACTION
	SET_PORT_AS_INPUT(A,1);
	SET_PORT_HIGH(A,1);	//PULL UP

	SET_PORT_AS_INPUT(A,2);
	SET_PORT_HIGH(A,2); //PULL UP


	//DEFINE INTERRUPT FOR BUTTON ON PE4 and PE5
	//We need interrupt only on falling edge (button pressed)
	SB_LOW(EICRB,ISC41);
	SB_HIGH(EICRB,ISC40);
	SB_LOW(EICRB,ISC51);
	SB_HIGH(EICRB,ISC50);
	//Activate the interrupt
	SB_HIGH(EIMSK,INT4);
	SB_HIGH(EIMSK,INT5);
	// Caption of interrupt are at the end of main.c ISR(INT4_vect) and ISR(INT5_vect)

	//DEBUG PORT FOR LOGIC ANALYSER
	SET_PORT_AS_OUTPUT(A,6);
	SET_PORT_AS_OUTPUT(A,5);
	SET_PORT_HIGH(A,5);
	SET_PORT_HIGH(A,6);

	//THIS IS TEST WILL NOT STAY
	//SET_PORT_AS_INPUT(E,0);
	//SET_PORT_LOW(E,0);


	SET_PORT_HIGH(A,7);		//LIGHT THE LED TO SAY HELLO :)

	//serial1_writestring("Vincent el cador");

	while(1==1)
	{

		_delay_ms(500);
		 SET_PORT_LOW(A,7);
		 _delay_ms(500);
		 SET_PORT_HIGH(A,7);
	}

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
	mode=MODE_FLY;

	switch(mode)
	{

		case MODE_FLY:
		case MODE_FLY_FPV:
		case MODE_FLY_FPV_MIX:
			SET_PORT_HIGH(A,0); 	 	//WE LIGHT UP MAX232 VCC
			Init_FrSky();				//Initialise Cnx with Frsky Module
			set_evo_rssi_alarm_level(RSSI_EVO_ALARM);	//Royal Evo RSSI Alarm Level See define for more info
			init_royal(standard_boot);	//Initialise Cnx with Royal Evo (standard boot means we will wait for real negotiation with royal evo otherwise if we cannot akwnoledge Royal evo we skip try several nego and then start directly to listening to channel position stream
			init_ppm();					//Initialise PPM Writer

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

					if(serial0_input_writect==0)

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

						 set_evo_telemetry(0,UNIT_LQI,	get_FrSky_rssi_up_link()				,0);

						 set_evo_telemetry(1,UNIT_LQI,	get_FrSky_rssi_down_link()				,0);

						 set_evo_telemetry(2,UNIT_V,	get_FrSky_sensor1()*3.3*10*4/256		,0);

						 set_evo_telemetry(3,UNIT_V,	get_FrSky_sensor2()*3.3*10*4/256		,0);





					 _delay_ms(15);
					 // We wait for ppm frame to be done until that
					 // as we do not have nothing better to do we decode
					 // incoming information from FrSky()
					 // while (!is_ppm_active())Read_FrSky();



					 send_evo_telemetry();	//SEND THE TELEMETRY TO ROYAL EVO

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

			SET_PORT_AS_OUTPUT(D,2);
			SET_PORT_LOW(D,2);

			SET_PORT_AS_INPUT(D,3);
			SET_PORT_LOW(D,3);

			SET_PORT_AS_INPUT(A,5);
			SET_PORT_LOW(A,5);

			SET_PORT_AS_OUTPUT(A,6);
			SET_PORT_LOW(A,6);

			while(TRUE)
			{
				//COPY STATE OF USB OUTPUT TO ROYAL INPUT
				if(READ_PORT_INPUT(D,3))
					SET_PORT_HIGH(A,6);
				else
					SET_PORT_LOW(A,6);

				//COPY STATE OF ROYAL OUTPUT TO USB INPUT
				if(READ_PORT_INPUT(A,5))
					SET_PORT_HIGH(D,2);
				else
					SET_PORT_LOW(D,2);
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


//Button Interrupt
ISR(INT4_vect)
{
	if(evo_bt1%2)
	{
		evo_bt1_timestamp=0;
	}
		evo_bt1++;
}
ISR(INT5_vect)
{
	if(evo_bt2%2)
		{
			evo_bt2_timestamp=0;
		}
			evo_bt2++;
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

