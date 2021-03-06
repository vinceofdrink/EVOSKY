/*
 * File:   write_ppm.c
 * Author: vince
 *
 * Created on 16 mars 2011, 00:14
 *
 * This functions perform the generation of PPM Stream using TIMER2 to schedule event.
 * As an upgrade i plan to allow the generation of 2 distinct PPM stream using the COMP_B register comparator
 * So do not try to set NUMBER_OF_PPM_SIGNAL at 2
 *
 *
 *
 *
 *
 *
 *pulse[i]=1500+(MPX_voie[i]*16/44);

16/44 is the coefficient I determine with my oscilloscope !
With this coefficient I didn't have to change the setup of my RC models....

---
MPX pulse are (at 100%) from 950 to 2050 microseconds (center=1500)
so : 550 microseconds = 1520 in MPX protocol
so : coefficient = 550/1520 = 16/44 (approx. but permit to use 16 bits signed values.....)
 *
 *
 *% on radio -> datas -> original PPM -> restituted PPM (if my algorithm works)
-110% -> -1937
-100% -> -1761 -> 950 �s -> 953 �s
0% -> 0 -> 1500 �s -> 1500 �s
+100% -> 1760 -> 2050 �s -> 2045 �s
+110% -> 1936



OK, disassembled the EVO 3.41 code. The formula for converting the MLINK pulse output for each channel is:

((signed output value + 1521) * .361842) + 950

That gives you the 100% correct value. The range is hard coded at 950us to 2050us.

 * Typical usage is
 *
 */
#define WRITE_PPM_C
#include "write_ppm.h"
#include "macro_atmega.h"
// OPTION
//#define PPM_PIN_OSCILLOSCOPE_SYNC 4                          //Uncomment if you need a rising edge on the pin of your choice for oscilloscope Sync Trigger
//#define PPM_2_SIGNAL                                         //Uncomment if you need to generate 2 distincts PPM signal

//NOT TESTED ALREADY
#define NUMBER_OF_PPM_SIGNAL 1                                 //You can handle up to 2 distincts PPM signals at the same time



//PULSES MODULATION
#if F_CPU == 16000000
  #define CT_TO_US                        2                  // 2 for prescaler 8
#elif F_CPU == 18432000
  #define CT_TO_US                        2.304              //  2.304 for prescaler 8
#elif F_CPU == 8000000
  #define CT_TO_US                        1                  //  1 for prescaler 8
#elif F_CPU == 11059200
#define CT_TO_US                          1.3824
#elif F_CPU == 7372800
#define CT_TO_US						  0.92159
#endif

#define PPM_PULSE						400   * CT_TO_US   //in us PULSE
#define PPM_NEUTRAL						1100  * CT_TO_US   //in us NEUTRAL_POSTION = 1500-PULSE
#define PPM_FULL_FRAME                  17000 * CT_TO_US   //in us Length of a full PPM frame
#define PPM_OFFSET                      1 * CT_TO_US       //in us Wait after ppm_write for frame begin at least 1

#define PPM_DELAY_FOR_NEW_FRAME         5000*CT_TO_US                //Amount of time up wich we assume that the next incoming rising will be a new frame
#define PPM_DELTA_BETWEEN_PPM           (PPM_PULSE+PPM_NEUTRAL)      //deta between Rising edge to remove to get proper PPM Value


//GLOBAL VARIABLE
int g_chanel1[MAX_CHANEL_NUMBER];                           // channels encoded from -500 to 500 (middle point 0) (its litteraly the offset around middle point in us)
unsigned int  g_ppm1_timing[(MAX_CHANEL_NUMBER*2)+2];       // Store all the toogle point to draw a full ppm frame
unsigned char g_ppm1_ct      = 0;                          // compare point index for currently active
volatile unsigned char g_ppm_active   =0;

//EXTRA GLOBAL FOR PPM2 SIGNAL
#if defined(PPM_2_SIGNAL)
 unsigned int compB[(MAX_CHANEL_NUMBER*2)+2];           // 9 compare points for Timer2
 unsigned char iCompB      = 0;    // compare point index for currently active
#endif


//Setup the PPM (Call once in your script before use)
void init_ppm(void)
{
    
   
    SET_PORT_AS_OUTPUT(B,5);
   #if defined(PPM_2_SIGNAL)
    pinMode(10, OUTPUT);
   #endif

   #if defined(PPM_PIN_OSCILLOSCOPE_SYNC)
    pinMode(PPM_PIN_OSCILLOSCOPE_SYNC, OUTPUT);
    digitalWrite(PPM_PIN_OSCILLOSCOPE_SYNC, LOW);
   #endif

   //Initialize all chanel to 0
   unsigned char i;
   for(i=0; i != MAX_CHANEL_NUMBER ; i++)
        g_chanel1[i] = 0;

   //Set Timer to operate in normal mode
   ooTIMER1_NORMAL_MODE;
   //Set Timer in Toogle mode for PPM1 (pin 9)

   //REVERSE SIGNAL
   // ooTIMER1_TOOGLE_SET ;
   // TCCR1C |= (1<<FOC1A);


}

//PB5 A COMP
//PB6 B COM
//Write a ppm frame
void write_ppm(void)
{
	//We do not allow writing new PPM since we havent finish to write the previous PPM
    if(g_ppm_active)
      return;

    unsigned int previous_change=PPM_OFFSET;
    unsigned char i;

    //Calculation of the timing datagram of the PPM
    for( i=0; i !=MAX_CHANEL_NUMBER; i++)
    {

        g_ppm1_timing[(i*2)] =  previous_change + PPM_PULSE ;
        previous_change=g_ppm1_timing[(i*2)+1] = g_ppm1_timing[(i*2)] + (g_chanel1[i]) + PPM_NEUTRAL;
    }
    g_ppm1_timing[(MAX_CHANEL_NUMBER*2)] =  previous_change + PPM_PULSE ;
    g_ppm1_timing[(MAX_CHANEL_NUMBER*2)+1] = PPM_FULL_FRAME  ;

    //Set the next compare point
    OCR1A   = PPM_OFFSET;
    //g_ppm1_timing[0];
    //Reset the Timer Counter to Zero
    ooTIMER1_CT = 0x00;
    //Enable Interrupt on Compare A
    //ooTIMER1_INT_A_COMP_START(); //TIMSK1 = 1 << OCIE1A ;
    TIMSK = 1 << OCIE1A ;
    ooTIMER1_COMP_A_TOOGLE;
    //Start The timer
    ooTIMER1_SCALE_8;
   g_ppm_active=1;

}


// Timer 1 Comparator A (use for PPM1 modulation)
ISR(TIMER1_COMPA_vect)
{


    #if defined(PPM_PIN_OSCILLOSCOPE_SYNC)
     if(g_ppm1_ct==0)
        digitalWrite(PPM_PIN_OSCILLOSCOPE_SYNC, HIGH);
    #endif
    if( g_ppm1_ct == (MAX_CHANEL_NUMBER*2)+2 )
    {

      //Reset the Timer Counter and  step counter to Zero
      ooTIMER1_CT = 0x00;
      g_ppm1_ct  = 0;
      g_ppm_active =0;
      //Stop the timer
     ooTIMER1_STOP;
       #if defined(PPM_PIN_OSCILLOSCOPE_SYNC)

       digitalWrite(PPM_PIN_OSCILLOSCOPE_SYNC, LOW);
      #endif
     //dothetelemetry();
    }
    else
    {
      //Move to the next compare point
     if(g_ppm1_ct==(MAX_CHANEL_NUMBER*2))
         ooTIMER1_COMP_A_LOW;
      OCR1A = g_ppm1_timing[g_ppm1_ct];
      g_ppm1_ct++;
    }
}


