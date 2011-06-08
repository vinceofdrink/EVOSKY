/***********************************************************************************************************/
// This Section handle all the transaction IN & OUT Between Arduino and Royal Evo                         
// Almost all the code is derivated or copied from the excellent work of YannickF Project MPX2PPM        
// http://www.yannickf.net/spip/spip.php?article106
// 
// Some acces to the royal evo chanel have been done in macro to avoid function call
/***********************************************************************************************************/
#ifndef royal_evo.h
#define royal_evo.h
struct royal_tememetry_struct
{
    unsigned char unite;
    signed int valeur;
    unsigned char alarme;
};

extern royal_tememetry_struct  royal_tele[14];
//define pour la télémétrie
#define UNIT_V		1
#define UNIT_A		2
#define UNIT_MS		3
#define UNIT_KMH	4
#define UNIT_RPM	5
#define UNIT_DEGC	6
#define UNIT_DEGF	7
#define UNIT_M		8
#define UNIT_FUEL	9
#define UNIT_LQI	0x0A
#define UNIT_MAH	0x0B
#define UNIT_ML		0x0C
#define UNIT_D		0x0D
#define UNIT_E		0x0E
#define UNIT_F		0x0F

extern unsigned int evo_rssi;
extern signed int MPX_voie[16];

#define set_royal_evo_rssi(VALUE)  evo_rssi=VALUE
#define get_royal_chanel(VOIE)  MPX_voie[VOIE]

#define royal_evo_data_ready()  (data_counter>5 && (TCNT0-h_serial_timestamp)>2)  //Valid MPX TRAME start 0x80 and length 35
#define royal_evo_data_valid()  (data_counter==35 && data[0]==0x82)  // (194 ou 130); 0x80 dans le mode « Fast Response », 0x82 dans le mode normal, et 0x86 pour le réglage du Fail-Safe

#define set_evo_telemetry(POSITION, UNITE,VALEUR, ALARME) royal_tele[POSITION].unite=UNITE; royal_tele[POSITION].valeur=VALEUR;royal_tele[POSITION].alarme=ALARME
#endif
