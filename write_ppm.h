/* 
 * File:   write_ppm.h
 * Author: vince
 *
 * Created on 16 mars 2011, 00:14
 */
#define MAX_CHANEL_NUMBER 8                                    //Max number of channels
#ifndef WRITE_PPM_C

extern signed int      g_chanel1[MAX_CHANEL_NUMBER];
extern volatile unsigned char 	g_ppm_active;
//extern unsigned char  g_read_ppm_Timer_overflow;
//#define read_ppm_chanel(CHANEL)  read_ppm[CHANEL]

#define set_ppm1_chanel(CHANEL,VALUE)	g_chanel1[CHANEL]=VALUE
#define get_ppm1_chanel(CHANEL)	g_chanel1[CHANEL]
#define is_ppm_active() g_ppm_active
#endif

#ifndef WRITE_PPM_H
#define	WRITE_PPM_H


void init_ppm(void);
void write_ppm(void);

#endif	/* WRITE_PPM_H */

