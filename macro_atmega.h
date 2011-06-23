/***********************************************************************************************************/
// THIS FILE CONTAINS MACRO DEDICATE TO TIMER AND INTERRUPTION TO ATMEGA 128 / 168 / 328 
// I DO NOT EXPOSE ALL THE TIMER MODE, ONLY "NORMAL MODE" (CT COUNT FROM 0 TO TOP AND START AGAIN)
// Vincent de Boysson (vinceofdrink@gmail.com) Paris 2010
/***********************************************************************************************************/
// REMINDER OF SPI PORT
// ATMEGA 128 PB3->MISO PB2->MOSI PB1->SCK

#include <avr/interrupt.h>
#include <avr/io.h>




#define RESET_PORT(PORT_NAME)                       DDR##PORT_NAME =0x00;PORT##PORT_NAME=0XFF
#define SET_PORT_AS_INPUT(PORT_NAME,PORT_NUM)       SB_LOW(DDR##PORT_NAME,PORT_NUM)
#define SET_PORT_AS_OUTPUT(PORT_NAME,PORT_NUM)      SB_HIGH(DDR##PORT_NAME,PORT_NUM)
#define READ_PORT_INPUT(PORT_NAME,PORT_NUM)			PIN##PORT_NAME & (1 << PORT_NUM)

#define SET_PORT_HIGH(PORT_NAME,PORT_NUM)           SB_HIGH(PORT##PORT_NAME,PORT_NUM)
#define SET_PORT_LOW(PORT_NAME,PORT_NUM)            SB_LOW(PORT##PORT_NAME,PORT_NUM)

#define TOOGLE_PORT(PORT_NAME,PORT_NUM)				PORT##PORT_NAME ^= (1 << PORT_NUM)
#define READ_PORT(PORT_NAME,PORT_NUM)				PORT##PORT_NAME & (1 << PORT_NUM)

#define READ_BIT(TARGET,BIT)						TARGET&(1<<BIT)

#define SB_HIGH(TARGET,BIT)   TARGET|=_BV(BIT)      // SET BIT TO 1
#define SB_LOW(TARGET,BIT)    TARGET&=~_BV(BIT)     // SET BIT TO 0
#define sbi(TARGET,BIT)       TARGET|=_BV(BIT)      // SET BIT TO 1
#define cbi(TARGET,BIT)       TARGET&=~_BV(BIT)     // SET BIT TO 0
//MISC STUFF

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE 	0
#endif

//###################################################################################
//# HARDWARE INTERUPTION ON PIN 2 & 3
//###################################################################################

//PD1 et PD0 

//ON ATMEGA 128 PD3 PD2 ALSO USE FOR TX1 ET RX1
#define CHANGE 1
#define FALLING 2
#define RISING 3

#if (defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) )
#define interrupt_fct_pin2            SIGNAL(INT0_vect)
#define start_interrupt_pin2(MODE)    EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (MODE << ISC00);  EIMSK |= (1 << INT0)
#define stop_interrupt_pin2()         EIMSK &= ~(1 << INT0)
#define clean_interrupt_pin2()        SB_HIGH(EIFR,INTF0)

#define interrupt_fct_pin3            SIGNAL(INT1_vect)
#define clean_interrupt_pin3()        SB_HIGH(EIFR,INTF1)
#define stop_interrupt_pin3()         EIMSK &= ~(1 << INT1)
#define start_interrupt_pin3(MODE)    EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (MODE << ISC10); EIMSK |= (1 << INT1)
#endif

#if defined(__AVR_ATmega128__)
#define interrupt_fct_int_0           SIGNAL(INT0_vect)
#define start_interrupt_int_0(MODE)   EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (MODE << ISC00);  EIMSK |= (1 << INT0)
//EIMSK &= ~(1 << INT##INTNUM)
#define stop_external_interrupt(INTNUM)        SB_LOW(EIMSK,INT##INTNUM) 
#define clean_external_interrupt(INTNUM)       SB_HIGH(EIFR,INTF##INTNUM)
#endif

//###################################################################################
//# GROUPED HARDWARE INTERUPTION
//###################################################################################

// Abstractions for maximum portability between processors
// These are macros to associate pins to pin change interrupts
// Courtesy Paul Stoffregen
#if !defined(digitalPinToPCICR)
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
#define digitalPinToPCICR(p)    (((p) >= 0 && (p) <= 21) ? (&PCICR) : ((uint8_t *)NULL))
#define digitalPinToPCICRbit(p) (((p) <= 7) ? 2 : (((p) <= 13) ? 0 : 1))
#define digitalPinToPCMSK(p)    (((p) <= 7) ? (&PCMSK2) : (((p) <= 13) ? (&PCMSK0) : (((p) <= 21) ? (&PCMSK1) : ((uint8_t *)NULL))))
#define digitalPinToPCMSKbit(p) (((p) <= 7) ? (p) : (((p) <= 13) ? ((p) - 8) : ((p) - 14)))
#else
#define digitalPinToPCICR(p)    ((uint8_t *)NULL)
#define digitalPinToPCICRbit(p) 0
#define digitalPinToPCMSK(p)    ((uint8_t *)NULL)
#define digitalPinToPCMSKbit(p) 0
#endif
#endif


//###################################################################################
//# TIMER 0 ATMEGA 
//###################################################################################

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
    #define	ooTIMER0_STOP		        TCCR0B = 0<<CS02 | 0<<CS01 | 0<<CS00  //0
    #define	ooTIMER0_SCALE_1		TCCR0B = 0<<CS02 | 0<<CS01 | 1<<CS00  //16000000
    #define	ooTIMER0_SCALE_8		TCCR0B = 0<<CS02 | 1<<CS01 | 0<<CS00  //2000000
    #define	ooTIMER0_SCALE_64		TCCR0B = 0<<CS02 | 1<<CS01 | 1<<CS00  //500000
    #define	ooTIMER0_SCALE_256		TCCR0B = 1<<CS02 | 0<<CS01 | 0<<CS00  //250000
    #define	ooTIMER0_SCALE_1024		TCCR0B = 1<<CS02 | 0<<CS01 | 1<<CS00
    #define ooTIMER0_CT                     TCNT0
    #define ooTIMER0_COMP_A                 OCR0A
    #define ooTIMER0_COMP_B                 OCR0B

    #define ooTIMER0_INT_OVER               ISR(TIMER0_OVF_vect)
    #define ooTIMER0_OVERFLOW_ON            TIMSK0 |= (1<<TOIE0)
    #define ooTIMER0_COMP_A_ENABLE          SB_HIGH(TIMSK0,OCIE0A)
    #define ooTIMER0_OVERFLOW_OFF           SB_LOW(TIMSK0,TOIE0)
#endif

// A TESTER 
#if defined(__AVR_ATmega128__)

    // TOGLE OCCUR ON PIN PB3
    #define ooTIMER0_STOP		    TCCR0 = 0<<CS02 | 0<<CS01 | 0<<CS00  //0
    #define ooTIMER0_SCALE_1                TCCR0 = 0<<CS02 | 0<<CS01 | 1<<CS00  //16000000
    #define ooTIMER0_SCALE_8                TCCR0 = 0<<CS02 | 1<<CS01 | 0<<CS00  //2000000
    #define ooTIMER0_SCALE_32               TCCR0 = 0<<CS02 | 1<<CS01 | 1<<CS00  //500000
    #define ooTIMER0_SCALE_64               TCCR0 = 1<<CS02 | 0<<CS01 | 0<<CS00  //250000
    #define ooTIMER0_SCALE_128              TCCR0 = 1<<CS02 | 0<<CS01 | 1<<CS00
	#define ooTIMER0_SCALE_256              TCCR0 = 1<<CS02 | 0<<CS01 | 1<<CS00
    #define ooTIMER0_SCALE_1024             TCCR0 = 1<<CS02 | 1<<CS01 | 1<<CS00
    #define ooTIMER0_CT                     TCNT0
    #define ooTIMER0_COMP_A                 OCR0; 
    

    #define ooTIMER0_INT_OVER               void ISR(TIMER0_OVF_vect)
    #define ooTIMER0_OVERFLOW_ON            SB_HIGH(TIMSK,TOIE0)
    #define ooTIMER0_COMP_A_ENABLE          SB_HIGH(TCCR0, COM00);SB_LOW(TCCR0, COM01);
    #define ooTIMER0_OVERFLOW_OFF           SB_LOW(TIMSK,TOIE0)
#endif
//###################################################################################
//# TIMER 1
//###################################################################################
//ATMEGA 128
//PB6 OC1B (Output Compare and PWM Output B for Timer/Counter1)
//PB5 OC1A (Output Compare and PWM Output A for Timer/Counter1)

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega128__)
    #define ooTIMER1_COMP_A_TOOGLE  TCCR1A=0<<COM1A1 | 1<<COM1A0;
    //#define ooTIMER1_COMP_A_OFF  TCCR1A=0;
    #define ooTIMER1_COMP_A_LOW TCCR1A=1<<COM1A1 | 0<<COM1A0;
    //#define ooTIMER1_TOOGLE_A_HIGH  TCCR1A=1<<COM1A1 | 1<<COM1A0;


    //TCCR1B &= ~(1<<CS11);
    // unset a bit TCCR1B &= ~(1<<CS11);
    #define ooTIMER1_NORMAL_MODE            TCCR1A = 0;

    //#define ooTIMER1_INT_A_COMP(BIT)        TIMSK1|= BIT<<OCIE1A   // Output Compare A Match Interrupt Enable

    //#define ooTIMER1_INT_OVERFLOW_CT(BIT)   TIMSK1|= BIT<<TOIE1    // CT Overflow Interrupt Enable
    #define	ooTIMER1_CT			TCNT1
    #define     ooTIMER1_STOP                   TCCR1B = 0<<CS12 | 0<<CS11 | 0<<CS10  //(Timer/Counter stopped)

    #define ooTIMER1_SCALE_8                TCCR1B = 0<<CS12 | 1<<CS11 | 0<<CS10  //0
#endif

/*
//ACTION ON ARDUINO PIN 9
#define ooTIMER1_COMP_A_OFF       TCCR1A=1<<COM1A1 | 0<<COM1A0; //SB_LOW(TCCR1A,COM1A0);SB_LOW(TCCR1A,COM1A0)   //CLEAR ALL INTERACTION WITH PIN 9
#define ooTIMER1_COMP_A_TOOGLE   TCCR1A|= 0<<COM1A1 | 1<<COM1A0;               //TOGGLE FOR EACH MATCH THE PIN 9
#define ooTIMER1_COMP_A_LOW      TCCR1A|=1<<COM1A1 | 0<<COM1A0;                //SET OFF PIN 9 FOR EACH MATCH
#define ooTIMER1_TOOGLE_A_HIGH   TCCR1A|=1<<COM1A1 | 1<<COM1A0;                //SET ON PIN 9 FOR EACH MATCH
#define ooTIMER1_FORCE_COMP_A    TCCR1C |= (1<<FOC1A);                         //GENERATE A MATCH COMPARE (To be tested)


//ACTION ON ARDUINO PIN 10
#define ooTIMER1_COMP_B_OFF      SB_LOW(TCCR1A,COM1B1);SB_LOW(TCCR1A,COM1B0)   //CLEAR ALL INTERACTION WITH PIN 10
#define ooTIMER1_COMP_B_TOOGLE   TCCR1A|=0<<COM1B1 | 1<<COM1B0;                //TOGGLE FOR EACH MATCH THE PIN 10
#define ooTIMER1_COMP_B_LOW      TCCR1A|=1<<COM1B1 | 0<<COM1B0;                //SET OFF PIN 10 FOR EACH MATCH
#define ooTIMER1_TOOGLE_B_HIGH   TCCR1A|=1<<COM1B1 | 1<<COM1B0;                //SET ON PIN 10 FOR EACH MATCH
#define ooTIMER1_FORCE_COMP_B    TCCR1C |= (1<<FOC1B);



#define ooTIMER1_NORMAL_MODE            TCCR1A = 0;

#define ooTIMER1_INT_FNCT_A_COMP()     ISR(TIMER1_COMPA_vect)
#define ooTIMER1_INT_A_COMP_START()    TIMSK1|= 1<<OCIE1A   // Output Compare A Match Interrupt Enable
#define ooTIMER1_INT_A_COMP_STOP()     SB_LOW(TIMSK1,OCIE1A)  // Output Compare A Match Interrupt Enable

#define ooTIMER1_FNCT_B_COMP)          ISR(TIMER1_COMPB_vect)
#define ooTIMER1_INT_B_COMP_START()    TIMSK1|= 1<<OCIE1B   // Output Compare A Match Interrupt Enable
#define ooTIMER1_INT_B_COMP_STOP()     SB_LOW(TIMSK1,OCIE1B)  // Output Compare A Match Interrupt Enable

#define ooTIMER1_FNCT_OVERFLOW()       ISR(TIMER1_OVF_vect)
#define ooTIMER1_INT_OVERFLOW_START()  TIMSK1|= 1<<TOIE1   // Overflow Interupt
#define ooTIMER1_INT_OVERFLOW_STOP()   SB_LOW(TIMSK1,TOIE1)  // Output Compare A Match Interrupt Enable

//#define ooTIMER1_INT_OVERFLOW_CT(BIT)   TIMSK1|= BIT<<TOIE1    // CT Overflow Interrupt Enable
#define	ooTIMER1_CT			TCNT1
#define	ooTIMER1_COMP_A                 CTOCR1A
#define	ooTIMER1_COMP_B                 CTOCR1B

#define ooTIMER1_STOP                   TCCR1B = 0<<CS12 | 0<<CS11 | 0<<CS10  //(Timer/Counter stopped)
#define ooTIMER1_SCALE_1                TCCR1B = 0<<CS12 | 0<<CS11 | 1<<CS10  //0
#define ooTIMER1_SCALE_8                TCCR1B = 0<<CS12 | 1<<CS11 | 0<<CS10  //0
#define ooTIMER1_SCALE_64               TCCR1B = 0<<CS12 | 1<<CS11 | 1<<CS10  //0
#define ooTIMER1_SCALE_256              TCCR1B = 1<<CS12 | 0<<CS11 | 0<<CS10  //0
#define ooTIMER1_SCALE_1024             TCCR1B = 1<<CS12 | 0<<CS11 | 1<<CS10  //0
#define ooTIMER1_SCALE_EXT_FALL         TCCR1B = 1<<CS12 | 1<<CS11 | 1<<CS10  //External clock source on T1 pin. Clock on falling edge.
#define ooTIMER1_SCALE_EXT_RISING       TCCR1B = 1<<CS12 | 1<<CS11 | 1<<CS10  //External clock source on T1 pin. Clock on rising edge.
*/
//###################################################################################
//# TIMER 2
//###################################################################################

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
    #define ooTIMER2_INIT			TCCR2A = 0    //I

    #define	ooTIMER2_SCALE_0		TCCR2B = 0<<CS22 | 0<<CS21 | 0<<CS20  //0
    #define	ooTIMER2_SCALE_1		TCCR2B = 0<<CS22 | 0<<CS21 | 1<<CS20  //16000000
    #define	ooTIMER2_SCALE_8		TCCR2B = 0<<CS22 | 1<<CS21 | 0<<CS20  //2000000
    #define	ooTIMER2_SCALE_32		TCCR2B = 0<<CS22 | 1<<CS21 | 1<<CS20  //500000
    #define	ooTIMER2_SCALE_64		TCCR2B = 1<<CS22 | 0<<CS21 | 0<<CS20  //250000
    #define	ooTIMER2_SCALE_128		TCCR2B = 1<<CS22 | 0<<CS21 | 1<<CS20  //125000
    #define	ooTIMER2_SCALE_256		TCCR2B = 1<<CS22 | 1<<CS21 | 0<<CS20  //62500
    #define	ooTIMER2_SCALE_1024		TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20  //15625

    #define	ooTIMER2_START			TIMSK2 = 1<<TOIE2
    #define	ooTIMER2_STOP			TIMSK2 = 0<<TOIE2
    #define	ooTIMER2_CT			TCNT2

    #define	ooTIMER2_FUNCTION		ISR(TIMER2_OVF_vect)
#endif



//###################################################################################
//# ARDUINO 168 & 328 LAYOUT & MISC INFO
//###################################################################################
//
//
// Available servo channels:
// * 1 - OC1A (avr pin 15 PB1) - arduino digital pin 9 (high resolution)
// * 2 - OC1B (avr pin 16 PB2) - arduino digital pin 10 (high resolution)
// * 3 - OC2A (avr pin 17 PB3) - arduino digital pin 11 (low resolution)
// * 4 - OC2B (avr pin  5 PD3) - arduino digital pin 3 (low resolution)
//
// ATMEL ATMEGA8 & 168 / ARDUINO
//
//                  +-\/-+
//            PC6  1|    |28  PC5 (AI 5)
//      (D 0) PD0  2|    |27  PC4 (AI 4)
//      (D 1) PD1  3|    |26  PC3 (AI 3)
//      (D 2) PD2  4|    |25  PC2 (AI 2)
// PWM+ (D 3) PD3  5|    |24  PC1 (AI 1)
//      (D 4) PD4  6|    |23  PC0 (AI 0)
//            VCC  7|    |22  GND
//            GND  8|    |21  AREF
//            PB6  9|    |20  AVCC
//            PB7 10|    |19  PB5 (D 13)
// PWM+ (D 5) PD5 11|    |18  PB4 (D 12)
// PWM+ (D 6) PD6 12|    |17  PB3 (D 11) PWM
//      (D 7) PD7 13|    |16  PB2 (D 10) PWM
//      (D 8) PB0 14|    |15  PB1 (D 9) PWM
//                  +----+
//

