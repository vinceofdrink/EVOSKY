
#ifndef READ_PPMC
extern volatile signed  g_read_ppm[];
#define read_ppm_chanel(CHANEL)  	g_read_ppm[CHANEL]
#endif
void init_read_ppm(unsigned char ); // Argument is 0 for PPM negative and 1 for positive
