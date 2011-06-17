extern unsigned char g_FrSky_rssi_up_link;       // RSSI Value UP-LINK (RADIO TO MODEL)
extern unsigned char g_FrSky_rssi_down_link;       // RSSI Value DOWN-LINK (RX TELEMETRY TO RADIO)
extern unsigned char g_FrSky_sensor1;     // Value of Sensor 1
extern unsigned char g_FrSky_sensor2;     // Value of Sensor 2

//MACRO TO AVOID FUNCTION CALL THAT COST A BIT IN PROC
#define get_FrSky_rssi_up_link() 		g_FrSky_rssi_up_link
#define get_FrSky_rssi_down_link() 		g_FrSky_rssi_down_link
#define get_FrSky_sensor1() 	g_FrSky_sensor1
#define get_FrSky_sensor2()  	g_FrSky_sensor2
void Init_FrSky(void);
unsigned char   NewUserDataFrSky(void);
unsigned char 	ReadUserDataFrSky(void);
void Read_FrSky(void);

