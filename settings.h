/*
 * This is included in every project file
 * It group all the setting you need to change to ajust the project to your need
 *  Created on: 11 juil. 2011
 *      Author: vince
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

/*GENERAL SETTING*/
#define NB_CHANEL_EVO 		9
#define NB_CHANEL_TELEMETRY 		16
#define NB_MAX_MODEL_MEMORY			10
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

// Double click must be inside the timming to be valid the more the value is high the more you can take your time to execute the double click
#define  BUTTON_DOUBLECLICK_TIMMING 	12  //1 =+- 20ms (PPM FRAME) 12 seem a acceptable value

#define ROYAL_EVO_UART		1
#define FRSKY_UART			0



//if you define MODEL_CHANEL_SELECTOR to a royal evo channel
//This will allow saving parameter to a specific model
// You will need to fixed value to the channel
// 1% to X% the % should be in relation with your model number
// I use in my case i user also negative value  so i can know if i want to use my FRSKY TX or CORONA TX this is specific to my setup
#define MODEL_CHANEL_SELECTOR	9



#endif /* SETTINGS_H_ */
