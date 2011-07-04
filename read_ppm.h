extern signed int g_read_ppm[];
#ifndef READ_PPMC
#define read_ppm_chanel(CHANEL)  	g_read_ppm[CHANEL]
#endif
void init_read_ppm(unsigned char ); // Argument is 0 for PPM negative and 1 for positive
