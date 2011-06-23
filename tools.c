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
