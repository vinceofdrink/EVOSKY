/***********************************************************************************************************/
// This Section is all about decoding an PPM input Stream
// By default we use interrupt 0 wich is related to Changing state of arduino PIN 3
// Author : Vincent de Boysson vinceofdrink@gmail.com
/***********************************************************************************************************/
#include "macro_atmega.h"
#include "read_ppm.h"


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

#define PPM_PULSE			300   * CT_TO_US   							//in us PULSE
#define PPM_NEUTRAL			1200  * CT_TO_US   							//in us NEUTRAL_POSTION = 1500-PULSE
#define PPM_FULL_FRAME                  17000 * CT_TO_US   				//in us Length of a full PPM frame
#define PPM_OFFSET                      1 * CT_TO_US       				//in us Wait after ppm_write for frame begin at least 1

#define PPM_DELAY_FOR_NEW_FRAME         5000*CT_TO_US                	//Amount of time up wich we assume that the next incoming rising will be a new frame
#define PPM_DELTA_BETWEEN_PPM           (PPM_PULSE+PPM_NEUTRAL)      	//delta between Rising edge to remove to get proper PPM Value

signed int read_ppm[12];
unsigned char read_ppm_refresh =0; // Is set to one as soon as we read a new PPM Frame
unsigned char g_read_ppm_Timer_overflow=0;
unsigned int g_read_ppm_last_postion=0;
unsigned char g_read_ppm_pos=0;
void init_read_ppm(void)
{
  ////////////////////////REACTIVE L INTERRUPTION
  /////pinMode(3, INPUT);
  //attachInterrupt(1, ppm_rising, RISING);
  //start_interrupt_pin3(RISING);
 read_ppm[0]=0;
 OCR1B=0;
 
}
/*
ISR(TIMER2_OVF_vect)
{
  g_read_ppm_Timer_overflow++;
}
*/
SIGNAL(INT1_vect) 
{
  cli();
  OCR1BL=TCNT0;
  if(OCR1B>PPM_DELAY_FOR_NEW_FRAME)
  {
    g_read_ppm_pos=0;
  }
  else
  {
     read_ppm[g_read_ppm_pos]=OCR1B-PPM_NEUTRAL;
     if(g_read_ppm_pos!=12)
       g_read_ppm_pos++;
  }
   OCR1B=0;
   sei();
}



 
