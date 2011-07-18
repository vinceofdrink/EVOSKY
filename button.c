/*
 * button.c
 *
 *  Created on: 17 juil. 2011
 *      Author: vince
 *  Handle button interruption, soft deboucing and decoding click type .
 *  Allow detection of
 *  - Single click
 *  - Double click
 *  - Long Hold down
 *
 */


#ifndef BUTTON_C_
#define BUTTON_C_
#endif /* BUTTON_C_ */

#include "settings.h"
#include "button.h"
#include "macro_atmega.h"
#include "royal_evo.h"

volatile unsigned char evo_bt1=0;
volatile unsigned char evo_bt2=0;
volatile unsigned char evo_bt_timestamp=0;


void init_button()
{

		//BUTTON  PE4 PE5 defined as INPUT PULL UP
		SET_PORT_AS_INPUT(E,4);
		SET_PORT_HIGH(E,4);	//PULL UP

		SET_PORT_AS_INPUT(E,5);
		SET_PORT_HIGH(E,5); //PULL UP

		//DEFINE INTERRUPT FOR BUTTON ON PE4 and PE5

		//We need interrupt only on falling edge (button pressed)
		SB_HIGH(EICRB,ISC41);
		SB_LOW(EICRB,ISC40);
		SB_HIGH(EICRB,ISC51);
		SB_LOW(EICRB,ISC50);
		//Activate the interrupt
		SB_HIGH(EIMSK,INT4);
		SB_HIGH(EIMSK,INT5);
}

unsigned char get_button_state(void)
{

	if(READ_PORT_INPUT(E,4) && !READ_BIT(EIMSK,INT4)&& evo_bt_timestamp!=255)
	{
		SB_HIGH(EIFR,INTF4);
		SB_HIGH(EIMSK,INT4);
	}
	if(READ_PORT_INPUT(E,5) && !READ_BIT(EIMSK,INT5)&& evo_bt_timestamp!=255)
	{
			SB_HIGH(EIFR,INTF5);
			SB_HIGH(EIMSK,INT5);

	}

	if(evo_bt1==0 && evo_bt2==0)
		return BUTTON_NO_EVENT;

	evo_bt_timestamp++;
	if(evo_bt_timestamp<BUTTON_DOUBLECLICK_TIMMING  )
		return BUTTON_NO_EVENT;

	unsigned char return_value=BUTTON_NO_EVENT;


	if(evo_bt1==1)
	{
		if(READ_PORT_INPUT(E,4) )
			return_value= BUTTON1_SINGLE_CLICK;
		else
			return_value= BUTTON1_LONG_CLICK;
	}


	if(evo_bt1==2)
		return_value= BUTTON1_DOUBLE_CLICK;

	if(evo_bt2==1)
	{
		if(READ_PORT_INPUT(E,5) )
			return_value= BUTTON2_SINGLE_CLICK;
		else
			return_value= BUTTON2_LONG_CLICK;
	}
	if(evo_bt2==2)
		return_value= BUTTON2_DOUBLE_CLICK;

	if(evo_bt1==1 && evo_bt2==1)
			return_value= BUTTON_BOTH_SINGLE_CLICK;

	evo_bt_timestamp=evo_bt1=evo_bt2=0;

	return return_value;
}



ISR(INT4_vect)
{
	SB_LOW(EIMSK,INT4); // Deactivate interrupt prevent boucing
	evo_bt_timestamp=255;
	evo_bt1++;
}
ISR(INT5_vect)
{
	SB_LOW(EIMSK,INT5); // Deactivate interrupt prevent boucing
	evo_bt_timestamp=255;
	evo_bt2++;
}





