/*
 * serial.h
 *
 *  Created on: 28 mai 2011
 *      Author: vince
 */
//Taille des buffers respectif
#define SERIAL0_BUFFER	100   // This are very conservative value for the project we could use much smaller buffer.
#define SERIAL1_BUFFER	100	  // This are very conservative value for the project we could use much smaller buffer.


#ifndef SERIAL_H_
#define SERIAL_H_

void serial0_init(unsigned long baud);
void serial0_close(void);
void serial0_writechar( unsigned char data );
void serial0_writestring(char * str);

void serial_0_change_rate(unsigned long baud);


void serial1_init(unsigned long baud);
void serial1_close(void);
void serial1_writechar( unsigned char data );
void serial1_writestring(char * str);
unsigned char serial0_readchar(void);
unsigned char serial1_readchar(void);

#define serial0_NewData()  serial0_input_writect!=serial0_input_readct
#define serial0_reset_ct()	serial0_input_writect=0;serial0_input_readct=0

//READ DIRECTLY AT THE SPECIFY X POSITION INTO THE CIRCULAR BUFFER OF serial0
#define serial0_direct_buffer_read(X)	serial0_input[X]


#define serial1_NewData()  serial1_input_writect!=serial1_input_readct
#define serial1_reset_ct()	serial1_input_writect=serial1_input_readct=0
#define serial1_direct_buffer_read(X)	serial1_input[X]
//
#if defined(SERIAL_C_)
unsigned char serial0_input[SERIAL0_BUFFER];
volatile unsigned char serial0_input_writect=0;
volatile unsigned char serial0_input_readct=0;

unsigned char serial1_input[SERIAL1_BUFFER];
volatile unsigned char serial1_input_writect=0;
volatile unsigned char serial1_input_readct=0;
#else
extern unsigned char serial0_input[SERIAL0_BUFFER];
extern volatile unsigned char serial0_input_writect;
extern volatile unsigned char serial0_input_readct;

extern unsigned char serial1_input[SERIAL1_BUFFER];
extern volatile unsigned char serial1_input_writect;
extern volatile unsigned char serial1_input_readct;
#endif /* SERIAL_C_ */

#endif /* SERIAL_H_ */
