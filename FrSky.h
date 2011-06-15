extern unsigned char g_FrSky_rssi1;       // RSSI Value
extern unsigned char g_FrSky_rssi2;       // RSSI Value
extern unsigned char g_FrSky_sensor1;    // Value of Sensor 1
extern unsigned char g_FrSky_sensor2;    // Value of Sensor 2

#define get_FrSky_rssi1() g_FrSky_rssi1
#define get_FrSky_rssi2() g_FrSky_rssi2
#define get_FrSky_sensor1()  g_FrSky_sensor1
void init_FrSky(void);
void  NewUserData(void);
void read_FrSky(void);
