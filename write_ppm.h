/* 
 * File:   write_ppm.h
 * Author: vince
 *
 * Created on 16 mars 2011, 00:14
 */
#define MAX_CHANEL_NUMBER 8                                    //Max number of channels
#ifndef WRITE_PPM_C

extern signed int      g_chanel1[MAX_CHANEL_NUMBER];
//extern unsigned char  g_read_ppm_Timer_overflow;
#define read_ppm_chanel(CHANEL)  read_ppm[CHANEL]
#endif

#ifndef WRITE_PPM_H
#define	WRITE_PPM_H


void init_ppm(void);
void write_ppm(void);

#endif	/* WRITE_PPM_H */

