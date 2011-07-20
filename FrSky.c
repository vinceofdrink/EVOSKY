/***********************************************************************************************************/
// This Section handle all the transaction From FRSky To AVR SERIAL INPUT
// Author : Vincent de Boysson vinceofdrink@gmail.com
/***********************************************************************************************************/
#include "settings.h"
#include "serial.h"
#include "tools.h"
#include "FrSky.h"
/**
 * USER CONSTANT
 */

#define	USER_DATA_BUFFER_SIZE	40			// Define the size of the circular buffer for user data

/**
 * GLOBAL YOU SHOULD NOT ACCES DIRECTLY TO THOSE AS I PROVIDE FUNCTION OR MACRO FOR THAT PURPOSE
 */
// Sensor & RSSI (Native to frsky module)
unsigned char g_FrSky_rssi_up_link	=	100;     // Store The Rssi Level
unsigned char g_FrSky_rssi_down_link=	100;     // Store The Rssi Level
unsigned char g_FrSky_sensor1=0;    		  // Store Value of Sensor 1
unsigned char g_FrSky_sensor2=0;    		  // Store Value of Sensor 2

// User data Buffer (the data that comes from RX will go there
unsigned char g_Frsky_user_buffer[USER_DATA_BUFFER_SIZE];		//Buffer
unsigned char g_Frsky_user_read_ct=0;							//Read Pointer
unsigned char g_Frsky_user_write_ct=0;							//Write Pointer

//Buffer to store and decode the frsky protocol input
unsigned char g_Frsky_ct=0;					//relate the position of the current byte of the current frame construction
unsigned char g_Frsky_buffer[12];  			// Buffer to store a Frsky Frame should need only  11 as we decode byte stuffing on the fly.

// Struct herited from lib "Tools.h" that perform a division by  4 on the last 4 input to obtain some stable value
struct moyenne_u_char  g_FrSky_rssi_up_link_moyenne,g_FrSky_rssi_down_link_moyenne;

// HERE the Brief for FRSKY Protocol
// 2 KIND OF FRAME
//
// TELEMETRY x7E 0xFE (SENSOR 1) (SENSOR 2) (RSSI_UP_LINK)  (RSSI_DOWN_LINK) (0) (0) (0) (0) x7E
// USER DATA 0x7E 0xFD (NUMBER OF BYTE) (FRAME COUNTER) (DATA 1) (DATA 2) (DATA 3) (DATA 4) (DATA 5) (DATA 6) x7E
//
// (FRAME COUNTER) SEEM TO SOMETIME BE SEND TWICE WITH THE SAME VALUE DONT NOW WHAT TO DO WITH IT I DONT USE IT
//
// RSSI_UP_LINK is used as provided
// RSSI_DOWN_LINK should as far as i know be divised by 2 to have a correct 0-100% value
// Byte stuffing decoding
//Byte in frame has value 0x7E is changed into 2 bytes: 0x7D 0x5E 
//Byte in frame has value 0x7D is changed into 2 bytes: 0x7D 0x5D

// My logic use the first and the last byte as the opening sequence for decoding (i know i will loose the first Frame from FRSKY how care)
// So position of the constant are according to this strategie.
#define FRSKY_POS_SENSOR_1 				3
#define FRSKY_POS_SENSOR_2 				4
#define FRSKY_POS_RSSI_UP_LINK  		5
#define FRSKY_POS_RSSI_DOWN_LINK  		6
#define FRSKY_POS_FRAME_TYPE			2
#define FRSKY_HEX_FRAME_START			x7E

void Init_FrSky(void)
{
   // Setup The Virtual UART Connexion ( Thanks to  http://arduiniana.org/libraries/newsoftserial/ )
	frsky_uart_init(9600);
	init_moyenne(&g_FrSky_rssi_down_link_moyenne,g_FrSky_rssi_down_link);
	init_moyenne(&g_FrSky_rssi_up_link_moyenne,g_FrSky_rssi_up_link);

}
void close_FrSky(void)
{

	frsky_uart_close();
  
}



unsigned char  NewUserDataFrSky(void)
{
	return g_Frsky_user_read_ct!=g_Frsky_user_write_ct;
}
unsigned char ReadUserDataFrSky(void)
{
	unsigned char tmp;
	tmp=g_Frsky_user_buffer[g_Frsky_user_read_ct++];
	if(g_Frsky_user_read_ct==USER_DATA_BUFFER_SIZE)
		g_Frsky_user_read_ct=0;

	return tmp;
}
/**
 * I do read frame as starting with 2 0x7E (wich correspond in fact at the end of the other frame and the beginning of the new one
 * We read all what we can from the input buffer or USART1
 */
void Read_FrSky(void)
{
  unsigned char current_char;
  unsigned char i;
  
  
  while( frsky_uart_NewData())
  {
     current_char= frsky_uart_readchar();

     //This part handle a suposed good frame completed
     if(g_Frsky_ct==11) //We shoud have a correct count for the frame
       {
         //we have a winner the frame is correct
         g_Frsky_ct=0; //Reset the counter


         if( current_char==0x7E)//If we have the correct ending
         {
         //We Handle the frame
          if(g_Frsky_buffer[2]==0xFE) //TELEMETRY FRAME
           {


        	  g_FrSky_sensor1  	 = g_Frsky_buffer[3];    // Store Value of Sensor 1
        	  g_FrSky_sensor2  = g_Frsky_buffer[4];    // Store Value of Sensor 2
        	  //g_FrSky_rssi_up_link  =  g_Frsky_buffer[5];    //RSSI UP-LINK
        	  g_FrSky_rssi_up_link = add_and_get_moyenne(&g_FrSky_rssi_up_link_moyenne, g_Frsky_buffer[5]);
        	  g_FrSky_rssi_down_link  =  add_and_get_moyenne(&g_FrSky_rssi_down_link_moyenne, g_Frsky_buffer[6]/2); 	  //RSSI DOWN-LINK as to be divised by 2
           }
          if(g_Frsky_buffer[2]==0xFD) //USER DATA FRAME
             {
        	  	  //The number of byte should be in the range of 1-6
        	  	  if(g_Frsky_buffer[3]>0 && g_Frsky_buffer[3]<7)
        	  	  {
        	  		  	// We Add the bytes to the user_data_buffer
        	  		  	for(i=0;i<g_Frsky_buffer[3];i++)
        	  		  	{
        	  		  		g_Frsky_user_buffer[g_Frsky_user_write_ct++]=g_Frsky_buffer[4+i];
        	  		  		//Handling the circular buffer avoiding overflow
        	  		  		if(g_Frsky_user_write_ct==USER_DATA_BUFFER_SIZE)
        	  		  			g_Frsky_user_write_ct=0;
        	  		  	}
        	  	  }
             }
         }
       }
      
     //This part construct the frame
     switch (g_Frsky_ct)
     {
       case 0:  // FIRST 0x7E
         if(current_char==0x7E)
           {
             g_Frsky_buffer[0]=current_char;
             g_Frsky_ct=1;
           }
       break;
       case 1: // SECOND 0x7E
          if(current_char==0x7E)
           {
             g_Frsky_buffer[1]=current_char;
             g_Frsky_ct=2;
           }
           else
           g_Frsky_ct=0;
       break;
       case 2: // FRAME TYPE 0x7E
           //We Handle 0xFE for télémetry and 0xFD for user data
           if( current_char == 0xFE || current_char == 0xFD)
           {
              g_Frsky_buffer[2]=current_char;
              g_Frsky_buffer[3]=0; // We clear the next char (useful for the byte stuffing detection and traduction)
              g_Frsky_ct=3;
           }
       break;
       default:
           //Here Come the data we have to manage the infamous Byte Stuffing
           if( g_Frsky_buffer[g_Frsky_ct]==0x7D) //TEST IF THE PREVIOUS CHAR WAS A 0x07D
           {
              if(current_char== 0x5E)
                {
                 g_Frsky_buffer[g_Frsky_ct]= 0x7E;
                 g_Frsky_ct++;
                }
             else if(current_char== 0x5D)
                {
                   g_Frsky_buffer[g_Frsky_ct]= 0x7D;
                   g_Frsky_ct++;
                }
              else
              {
                //We are in Trouble here after 0x7D we should only have 0x5E or 0x5D, problable jerk in the serial reception we cancel the frame
            	//We will Try to catch another Frame Start so we reset the g_Frsky_ct
                g_Frsky_ct=0;
              }
           }
           else
           {
              g_Frsky_buffer[g_Frsky_ct]=current_char;
              g_Frsky_buffer[g_Frsky_ct+1]= 0;  //We clean the next byte
              if(current_char!=0x7D) // Here come the joker the byte is stuffed need next char to guess the good value
            	  	 g_Frsky_ct++;
           }
           
       break;
     }
       
    
  }

}







