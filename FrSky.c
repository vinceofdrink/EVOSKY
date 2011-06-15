/***********************************************************************************************************/
// This Section handle all the transaction From FRSky To AVR SERIAL INPUT
// Author : Vincent de Boysson vinceofdrink@gmail.com
/***********************************************************************************************************/
#include "serial.h"



unsigned char g_FrSky_rssi1=100;     // Store The Rssi Level 
unsigned char g_FrSky_rssi2=100;     // Store The Rssi Level 
unsigned char g_FrSky_sensor1=0;    // Store Value of Sensor 1
unsigned char g_FrSky_sensor2=0;    // Store Value of Sensor 2

unsigned char g_Frsky_buffer[12];  // Buffer to store a Frsky Frame should need only  11 as we decode byte stuffing on the fly.

// Define the size of the circular buffer for user data
#define	USER_DATA_BUFFER	100


unsigned char g_Frsky_user_buffer[USER_DATA_BUFFER];
unsigned char g_Frsky_user_read_ct=0;
unsigned char g_Frsky_user_write_ct=0;

unsigned char g_Frsky_ct=0;

// HERE the Brief for FRSKY Protocol
// TELEMETRY x7E 0xFE (SENSOR 1) (SENSOR 2) (RSSI)  (??RSSI) (0) (0) (0) (0) x7E
// USER DATA 0x7E 0xFD (NUMBER OF BYTE) (NOT USE) (DATA 1) (DATA 2) (DATA 3) (DATA 4) (DATA 5) (DATA 6) x7E 

//Byte in frame has value 0x7E is changed into 2 bytes: 0x7D 0x5E 
//Byte in frame has value 0x7D is changed into 2 bytes: 0x7D 0x5D



void init_FrSky(void)
{
   // Setup The Virtual UART Connexion ( Thanks to  http://arduiniana.org/libraries/newsoftserial/ )
	serial1_init(9600);
   // attachInterrupt(, ppm_rising, RISING);
}
void close_FrSky(void)
{

	serial1_close();
  
}



void  NewUserData(void)
{
	return g_Frsky_user_read_ct!=g_Frsky_user_write_ct;
}

void read_FrSky(void)
{
  unsigned char current_char;
  
  
  
  while( serial1_NewData())
  {
     current_char= serial1_readchar();

     if(g_Frsky_ct==11)
       {
         //we have a winner the frame is correct
         g_Frsky_ct=0;
         if( current_char==0x7E)
         {
         //We Handle the frame
          if(g_Frsky_buffer[2]==0xFE) //TELEMETRY
           {
                  g_FrSky_sensor1 = g_Frsky_buffer[3];    // Store Value of Sensor 1
                  g_FrSky_sensor2 = g_Frsky_buffer[4];    // Store Value of Sensor 2
                  g_FrSky_rssi1  =  g_Frsky_buffer[5];     // Store The Rssi Level 
                  g_FrSky_rssi2  =  g_Frsky_buffer[6]; 
           }
          if(g_Frsky_buffer[2]==0xFD)//USER DATA
             {

             }
         }
       }
      
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
              g_Frsky_buffer[3]=0; // We clear the next char (useful for the byte stuffing detection)
              g_Frsky_ct=3; 
           }
       break;
       default:
           //Here Come the data we have to manage the infamous Byte Stuffing
           if( g_Frsky_buffer[g_Frsky_ct]==0x7D)
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
                //We are in Trouble here problable jerk in the serial reception we cancel the frame
                g_Frsky_ct=0;
              }
           }
           else
           {
              g_Frsky_buffer[g_Frsky_ct]=current_char;
              if(current_char!=0x7D) // Here come the joker the byte is stuffed need next char to guess the good value
                 {
                   g_Frsky_ct++;
                   g_Frsky_buffer[g_Frsky_ct]= 0;  //We clean the next byte
                 }
           }
           
       break;
     }
       
    
  }

}







