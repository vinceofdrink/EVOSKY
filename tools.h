/***********************************************************************************************************/
// This is a collection of utility function
// Author : Vincent de Boysson vinceofdrink@gmail.com
/***********************************************************************************************************/
struct moyenne_u_char
{
    unsigned char ct;
    unsigned char data[4];
};
struct last_stable_value
{
		unsigned char occurrence;
		signed int last_good;
		unsigned char ct;
		signed int challenger;
};


unsigned char add_and_get_moyenne(struct moyenne_u_char *,unsigned char);
void init_moyenne(struct moyenne_u_char *,unsigned char);

void init_last_stable_value(struct last_stable_value * ,unsigned char );
signed int compare_and_get_stable(struct last_stable_value * , signed int );

