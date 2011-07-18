/*
 * button.h
 *
 *  Created on: 17 juil. 2011
 *      Author: vince
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#ifndef BUTTON_C_
extern volatile unsigned char evo_bt1;
extern volatile unsigned char evo_bt2;
extern volatile unsigned char evo_bt_timestamp;
#endif

void init_button(void);
unsigned  char get_button_state(void);


//All the event we can obtain from user interaction
#define BUTTON_NO_EVENT				0
#define BUTTON1_SINGLE_CLICK		1
#define BUTTON1_DOUBLE_CLICK		2
#define BUTTON1_LONG_CLICK			3
#define BUTTON2_SINGLE_CLICK		4
#define BUTTON2_DOUBLE_CLICK		5
#define BUTTON2_LONG_CLICK			6
#define BUTTON_BOTH_SINGLE_CLICK	7



#define evosky_button2_click()	(evo_bt2!=0 && evo_bt2_timestamp++>=BUTTON_DOUBLECLICK_TIMMING)
#define evosky_button1_double_click()		(evo_bt1==2)
#define evosky_button2_double_click()  		(evo_bt2>2)
#define evosky_button1_long_click()			(evo_bt1==1)
#define evosky_button2_long_click() 		(evo_bt2==1)
#define evosky_button_reset()				evo_bt1=evo_bt2=0;


#endif /* BUTTON_H_ */
