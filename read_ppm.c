/***********************************************************************************************************/
// This Section is all about decoding an PPM input Stream
//
// Author : Vincent de Boysson vinceofdrink@gmail.com
/***********************************************************************************************************/

#define READ_PPMC
// IF YOU USE PPM INPUT BE SHURE TO THAT THE INPUT IS CONNECTED TO SOMETHING
// LETTING THE PIN FLOATING COULD GENERATE A LOT OF INTERRUPTION THAT COULD OVERFLOW OUR PROCESSOR CAPACITY
// IF THAT HAPPEND THE PPM READER WILL TRY TO TERMINATE OPERATING TO AVOID THAT SCENARIOS OF ALTERING THE OTHER FUNCTION OF
// EVOSKY
#include "macro_atmega.h"
#include "read_ppm.h"


#define NUMBER_OF_CHANEL_PPM_INPUT 	8

//PULSES MODULATION
#if F_CPU == 16000000
  #define CT_TO_US                        2                  // 2 for prescaler 8 
#elif F_CPU == 18432000 
  #define CT_TO_US                        2.304              //  2.304 for prescaler 8 
#elif F_CPU == 8000000 
  #define CT_TO_US                        1                  //  1 for prescaler 8 
#elif F_CPU == 11059200
  #define CT_TO_US                        1.3824
#endif

//DEFINE THE SKELETON OF A PPM DATAGRAM
//

#define PPM_NEUTRAL_REAL				1500  * CT_TO_US
#define PPM_MAX_PULSE					2200  * CT_TO_US
#define PPM_PULSE						300   * CT_TO_US   							//in us PULSE
#define PPM_NEUTRAL						1200  * CT_TO_US   							//in us NEUTRAL_POSTION = 1500-PULSE
#define PPM_FULL_FRAME                  17000 * CT_TO_US   							//in us Length of a full PPM frame
#define PPM_OFFSET                      1 * CT_TO_US       							//in us Wait after ppm_write for frame begin at least 1

#define PPM_PULSE_MIN					200   * CT_TO_US
#define PPM_DELAY_FOR_NEW_FRAME         5000*CT_TO_US                	//Amount of time up wich we assume that the next incoming event will be a new frame
#define PPM_DELTA_BETWEEN_PPM           (PPM_PULSE+PPM_NEUTRAL)      	//delta between Rising edge to remove to get proper PPM Value

volatile signed int g_read_ppm[NUMBER_OF_CHANEL_PPM_INPUT];

//Instead of volatile variable we use Free register from TIMER3
//To be honest beside the fact of freeing memory i dont really know if it speed up the execution

volatile unsigned int g_ppm_timestamp=0;

volatile unsigned char g_read_ppm_ct=0;


/*
#define g_ppm_timestamp 	ooTIMER3_COMP_A
#define g_read_ppm_ct		OCR3CH
#define g_read_ppm_ready	OCR3CL
#define g_ppm_delta			ooTIMER3_COMP_B
*/

//#define R_PPM_TIMESTAMP	ooTIMER3_COMP_A		//We use a unu
/**
 * signal_type	0 if negative PPM 1 if positive PPM
 */
void init_read_ppm(unsigned char signal_type)
{
	unsigned char i;
	ooTIMER3_NORMAL_MODE;



	//ENABLE NOISE CANCELER ON THE INPUT TRIGGER (WAIT FOR 4 Oscillator Cycle to trigger the event) THIS SHOULD NOT HURT
	SB_HIGH(TCCR3B,ICNC3);

	//NEGATIVE OR POSITIVE PPM SIGNAL
	if(signal_type)
		SB_HIGH(TCCR3B,ICES3);	//CAPTURE RISING FRONT
	else
		SB_LOW(TCCR3B,ICES3); 	//CAPTURE FALLING FRONT


	//Reset the ppm table
	for(i=0;i!=NUMBER_OF_CHANEL_PPM_INPUT;i++)
		g_read_ppm[i]=0;

	//CHOOSE A PRESCALER FOR OUR TIMER3
		ooTIMER3_SCALE_8;

	//ENABLE INTERRUPT CAPTURE FOR TIMER 3
	SB_HIGH(ETIMSK,TICIE3);

}

void close_read_ppm(void)
{
	//DISABLE INTERRUPT CAPTURE FOR TIMER 3
	SB_LOW(ETIMSK,TICIE3);
	//STOP THE TIMER
	ooTIMER3_STOP;
}
//INTERRUPTION OCCURS ON STATE CHANGE ON PIN PE7 WICH RECEIVE OUR PPM STREAM
ISR(TIMER3_CAPT_vect)
{

	  cli();
	  ETIMSK &= ~(1<<TICIE3); // prevent re-entrance
	  sei();
	  unsigned int ppm_delta=ICR3-g_ppm_timestamp;

	  g_ppm_timestamp=ICR3;
		if(ppm_delta>PPM_DELAY_FOR_NEW_FRAME)
		{
			g_read_ppm_ct=0;

		}
		else
		{
			if(ppm_delta>PPM_PULSE_MIN && ppm_delta<PPM_MAX_PULSE)
			{
				g_read_ppm[g_read_ppm_ct++]=ppm_delta-PPM_DELTA_BETWEEN_PPM;
			if(g_read_ppm_ct==NUMBER_OF_CHANEL_PPM_INPUT)
			{
				g_read_ppm_ct=0;
			}
			}
			else
			{
				g_read_ppm_ct=0;

			}
		}
	cli();
	ETIMSK |= (1<<TICIE3);// prevent re-entrance
	sei();
}




 
