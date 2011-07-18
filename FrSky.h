#include "settings.h"
extern unsigned char g_FrSky_rssi_up_link;       	// RSSI Value UP-LINK (RADIO TO MODEL)
extern unsigned char g_FrSky_rssi_down_link;       	// RSSI Value DOWN-LINK (RX TELEMETRY TO RADIO)
extern unsigned char g_FrSky_sensor1;     			// Value of Sensor 1
extern unsigned char g_FrSky_sensor2;     			// Value of Sensor 2

//MACRO TO AVOID FUNCTION CALL THAT COST A BIT IN PROC
#define get_FrSky_rssi_up_link() 		g_FrSky_rssi_up_link
#define get_FrSky_rssi_down_link() 		g_FrSky_rssi_down_link
#define get_FrSky_sensor1() 	g_FrSky_sensor1
#define get_FrSky_sensor2()  	g_FrSky_sensor2
void Init_FrSky(void);
unsigned char   NewUserDataFrSky(void);
unsigned char 	ReadUserDataFrSky(void);
void Read_FrSky(void);

//FRSKY Binding to UART
#if FRSKY_UART == 0
#define frsky_uart_change_rate(X)			serial0_change_rate(X)
#define frsky_uart_init(X)					serial0_init(X)
#define frsky_uart_NewData()				serial0_NewData()
#define frsky_uart_readchar()				serial0_readchar()
#define	frsky_uart_writechar(X)				serial0_writechar(X)
#define	frsky_uart_writestring(X)			serial0_writestring(X)
#define	frsky_uart_direct_buffer_read(X)	serial0_direct_buffer_read(X)
#define frsky_uart_close()					serial0_close()
#endif
#if FRSKY_UART == 1
#define frsky_uart_change_rate(X)			serial1_change_rate(X)
#define frsky_uart_init(X)					serial1_init(X)
#define frsky_uart_NewData()				serial1_NewData()
#define frsky_uart_readchar()				serial1_readchar()
#define	frsky_uart_writechar(X)				serial1_writechar(X)
#define	frsky_uart_writestring(X)			serial1_writestring(X)
#define	frsky_uart_direct_buffer_read(X)	serial1_direct_buffer_read(X)
#define frsky_uart_close()					serial1_close()
#endif

