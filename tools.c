/***********************************************************************************************************/
// This is a collection of utility function
// Author : Vincent de Boysson vinceofdrink@gmail.com
/***********************************************************************************************************/
#include "tools.h"

//Will perform (last 4 values)/4 to obtain more stable value
//As
unsigned char add_and_get_moyenne(struct moyenne_u_char * thestruct,unsigned char data)
{


	thestruct->data[thestruct->ct++]=data;
	if(thestruct->ct==4)
		thestruct->ct=0;
	return (thestruct->data[0]+thestruct->data[1]+thestruct->data[2]+thestruct->data[3])>>2;
}

// Set default value to our structure to inititalise structure
void init_moyenne(struct moyenne_u_char * thestruct,unsigned char data)
{
	thestruct->ct=0;
	thestruct->data[0]=data;
	thestruct->data[1]=data;
	thestruct->data[2]=data;
	thestruct->data[3]=data;
}

//allow filtering on input value that must show be send x time (occurrence) to be accounted as good and take place as the last good value.

void init_last_stable_value(struct last_stable_value * thestruct,unsigned char occurrence)
{
	thestruct->occurrence=occurrence;
	thestruct->last_good=0;
	thestruct->ct=0;
	thestruct->challenger=0;
}

signed int compare_and_get_stable(struct last_stable_value * thestruct, signed int value)
{
	if(thestruct->last_good==value)
			return thestruct->last_good;

	if(value==thestruct->challenger)
		thestruct->ct++;
	else
	{
		thestruct->ct=0;
		thestruct->challenger=value;
	}
	if(thestruct->ct==thestruct->occurrence)
	{
		thestruct->ct=0;
		thestruct->last_good=thestruct->challenger;
	}
	return thestruct->last_good;

}
