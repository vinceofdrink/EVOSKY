/*
 * serial.h
 *
 *  Created on: 28 mai 2011
 *      Author: vince
 */

#ifndef SERIAL_H_
#define SERIAL_H_
void serial0_init(unsigned long baud);
void serial0_writechar( unsigned char data );
void serial1_init(unsigned long baud);
void serial1_writechar( unsigned char data );
#endif /* SERIAL_H_ */
