/***********************************************************************************************************/
// This Section handle all the transaction IN & OUT Between Arduino and Royal Evo                         
// Almost all the code is derivated or copied from the excellent work of YannickF Project MPX2PPM        
// http://www.yannickf.net/spip/spip.php?article106
// 
// Some acces to the royal evo chanel have been done in macro to avoid function call
/***********************************************************************************************************/
#include "settings.h"
#ifndef ROYAL_EVO_H_
#define ROYAL_EVO_H_

struct royal_tememetry_struct
{
    unsigned char unite;
    signed int valeur;
    signed int high_valeur;			//Store the highest value reccorded
    signed int low_valeur;			//Store the lowest valut reccorded
    unsigned char alarme;
};

struct royal_telmetry_memo_struct
{
	signed int 		alarm_level[NB_CHANEL_TELEMETRY];		//if negative value this will be take as a under the value alarm if positive it will be an over the value alarm
	unsigned char 	multiplier_value[NB_CHANEL_TELEMETRY];	//the value will be divided by 100 and used to multplie the current input value
	unsigned int	fpv_chanel_1_offset;
	unsigned int	fpv_chanel_2_offset;
	unsigned int 	fpv_chanel_1_max;
	unsigned int 	fpv_chanel_1_min;
	unsigned int 	fpv_chanel_2_max;
	unsigned int 	fpv_chanel_2_min;
};

#if defined(ROYAL_EVO_C_)
#else
extern struct royal_tememetry_struct  royal_tele[];
#endif

//Royal Evo Binding to UART this allow to swing RoyalEvo from UART0 to UART1
#if ROYAL_EVO_UART == 0
#define evo_uart_change_rate(X)			serial0_change_rate(X)
#define evo_uart_init(X)				serial0_init(X)
#define evo_uart_NewData()				serial0_NewData()
#define evo_uart_readchar()				serial0_readchar()
#define	evo_uart_writechar(X)			serial0_writechar(X)
#define	evo_uart_writestring(X)			serial0_writestring(X)
#define	evo_uart_direct_buffer_read(X)	serial0_direct_buffer_read(X)
#define evo_uart_input_writect			serial0_input_writect
#endif

#if ROYAL_EVO_UART == 1
#define evo_uart_change_rate(X)			serial1_change_rate(X)
#define evo_uart_init(X)				serial1_init(X)
#define evo_uart_NewData()				serial1_NewData()
#define evo_uart_readchar()				serial1_readchar()
#define	evo_uart_writechar(X)			serial1_writechar(X)
#define	evo_uart_writestring(X)			serial1_writestring(X)
#define	evo_uart_direct_buffer_read(X)	serial1_direct_buffer_read(X)
#define evo_uart_input_writect			serial1_input_writect
#endif


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

//Display Mode
#define EVO_DISPLAY_NORMAL	0
#define EVO_DISPLAY_HIGH	1
#define EVO_DISPLAY_LOW		2

extern unsigned int evo_rssi;
extern signed int MPX_voie[16];
extern unsigned char evo_tele_ct;


//FUNCTION DECLARATION
void init_royal(unsigned char standard_boot);
void decode_evo_data(void);
void end_evo_transaction(unsigned char c);
unsigned char init_evo_negotiation(unsigned char);
void send_bind(void);
void send_nobind(void);
void send_commonbind(void);
void send_range(void);
void send_evo_telemetry();
void set_evo_rssi_alarm_level(unsigned char );
void set_evo_display_mode(unsigned char );
unsigned char get_evo_display_mode(void );
void reset_telemetry(void);
void evo_cursor_down(void);
void evo_cursor_up(void);
unsigned char evo_cursor_active(void);
unsigned char evo_get_cursor_pos(void);
void evo_reset_input_selector(void);
signed int evo_input_selector_value(void);
void store_evo_model(unsigned char pos);
void init_evo_model_storage(unsigned char pos);

//MACRO DEFINED TO AVOID FUNCTION CALL THAT COST A BIT IN PROC
#define royal_trame_ok() OCR0==1
#define set_royal_evo_rssi(VALUE)  evo_rssi=VALUE
#define get_royal_chanel(VOIE)  MPX_voie[VOIE]

//#define royal_evo_data_ready()  (data_counter>5 && (TCNT0-h_serial_timestamp)>2)  //Valid MPX TRAME start 0x80 and length 35
//#define royal_evo_data_valid()  (data_counter==35 && data[0]==0x82)  // (194 ou 130); 0x80 dans le mode Â« Fast Response Â», 0x82 dans le mode normal, et 0x86 pour le rÃ©glage du Fail-Safe

#define set_evo_telemetry(POSITION, UNITE,VALEUR, ALARME) royal_tele[POSITION].unite=UNITE; royal_tele[POSITION].valeur=VALEUR;royal_tele[POSITION].alarme=ALARME;if(VALEUR>royal_tele[POSITION].high_valeur)royal_tele[POSITION].high_valeur=VALEUR;if(VALEUR<royal_tele[POSITION].low_valeur &&VALEUR!=0)royal_tele[POSITION].low_valeur=VALEUR;
#define set_evo_telemetry_debug(POSITION, UNITE,VALEUR, ALARME) royal_tele[POSITION].unite=UNITE; royal_tele[POSITION].valeur=VALEUR;royal_tele[POSITION].alarme=ALARME;
#define get_evo_telemetry_counter()	evo_tele_ct
#define reset_trigger_for_next_data()  ooTIMER0_CT=0;ooTIMER0_OVERFLOW_ON;ooTIMER0_SCALE_8;OCR0=0
#endif
