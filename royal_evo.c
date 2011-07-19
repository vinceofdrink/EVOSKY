/***********************************************************************************************************/
// This Section handle all the transaction IN & OUT Between Arduino and Royal Evo                         
// Almost all the code is derivated or copied from the excellent work of YannickF Project MPX2PPM        
// http://www.yannickf.net/spip/spip.php?article106
/*



0x80 fast response
0x82 normal mode
add 4 for fail-safe (0x84 or 0x86)
So, I confirm :
Fast-mode is on bit 1
Fail-safe is on bit 2
*/
/***********************************************************************************************************/
#include "settings.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#define ROYAL_EVO_C_
#include "write_ppm.h"
#include "serial.h"
#include "royal_evo.h"
#include "macro_atmega.h"





#define  ROYAL_MAX_CHANEL_VALUE 1950		// if channel value of royal evo is >ROYAL_MAX_CHANEL_VALUE or <ROYAL_MAX_CHANEL_VALUE then value is presumed not valid

#define MODE_NORMAL				0x82		//DEC 130
#define MODE_FAST				0x80		//DEC 128
#define MODE_FAIL_SAFE			0x86		//DEC 134


#define CHANEL_SELECTOR_THRESOLD	300		//DELTA OF VALUE (from royal evo chanel scale) NEED TO GUESS WICH STICK THE USER WILL USE TO MODIFY VALUE

signed int MPX_voie[16]; //Multiplex Data Chanel
signed int MPX_voie_selector[NB_CHANEL_EVO];
unsigned char MPX_voie_current_selector=0xFF;

char nbvoies=0; 
unsigned char emulation_mode=1;
unsigned int evo_rssi=100;
unsigned char evo_alarm=30;
unsigned char evo_tele_ct=0;		//COUNTER OF TELEMETRY TO SEND FOR EACH FRAME

//Display context variable
unsigned char evo_display_mode=EVO_DISPLAY_NORMAL;
unsigned char evo_cursor_blink=0;
unsigned char evo_cursor_blink_state=0;
unsigned char evo_cursor_timestamp=255;


unsigned char 	frame_counter=0;
unsigned int 	per_cycle_error=0;
unsigned char 	per_frame_error=0;
struct royal_tememetry_struct  royal_tele[NB_CHANEL_TELEMETRY];
struct royal_telmetry_memo_struct royal_memo;



/*
au dÈpart, la communication est ‡ 19200 baud
la radio envoie ´ v ª
le module rÈpond en s identifiant (nom, n∞ de version)
la radio envoie ´  ? ª pour obtenir les infos de frÈquence des modules 35 - 40 ou 72 MHz (pas de ´  ? ª pour le M-LINK)
le module rÈpond par une chaine indiquant la gamme de frÈquence, le numero de canal et la frÈquence utilisÈe
si on tourne la molette pour changer de frÈquence, la radio envoie ´ + ª ou ´ - ª
et le module rÈpond par la fameuse chaine avec le numero de canal et la frÈquence
le module envoie 3 octets dont la signification n est pas clair (canal libre d aprËs le scanner ?) suivie de 0x0D 0x0A (retour ‡ la ligne, retour chariot.
la radio envoie ´ b ª pour dÈclencher la procÈdure de binding ou ´ a ª si tout est prÍt (binding au choix de l utilisateur au dÈmarrage)
le dÈbit passe ‡ 115200 baud
le module rÈpond 0x00 0xFF 0x00 0x00 0x00 dËs qu il est prÍt, ou 0x20 0xFF 0x00 0x00 0x00 si le binding est en cours
la radio envoie les donnÈes au format suivant :
  un octet 0x80 ou contenant le nombre de voies transmises
  36 (ou 24) paires d octet contentant les temps de chaque voies sur 12 bits signÈs (infos ‡ prÈciser)
le module MLINK (en fait le rÈcepteur dans le modËle rÈduit) renvoie 5 octets aprËs chaque paquets de donnÈes... Ces 5 octets contiennent les information de tÈlÈmÈtrie, binding, portÈe. (voir le site de Markus Stoeckli)
avec un module MLINK, le premier octet vaut 0x80 dans le mode ´ Fast Response ª, 0x82 dans le mode normal, et 0x86 pour le rÈglage du Fail-Safe

Attention 128=Mode Fast


Valeurs radio	-110%	-100%	0%	+100%	+110%
temps PPM		n.m			950µs	1500µs	2050µs	n.m
data HFMS	-1937	-1761	0	1760	1936
data O24RCP		950	1500	2050

LINK from -1521 to +1520 : OK
but -1976 to +1975 is the protocol for 35/40-41/72 MHz module ?
 */
// The timer is activated after the intro sequence and is reset after each input event of UART0
// If we overflow we do presume that we have a new frame ready from royal evo or nothing and we will trigger again the timer (see main)
ISR(TIMER0_OVF_vect)
{
	ooTIMER0_STOP;
	OCR0=1;
	evo_uart_input_writect=0;
}
/******************************************
 * Storing and retreiving default alarm value and value adjustement variable
 */
void store_evo_model(unsigned char pos)
{
	eeprom_busy_wait();

	eeprom_update_block(&royal_memo,(uint8_t*)1+(sizeof(royal_memo)*pos),sizeof(royal_memo));
}

void init_evo_model_storage(unsigned char pos)
{
	unsigned char i;
	for(i=0;i!=NB_CHANEL_TELEMETRY;i++)
	{
		royal_memo.alarm_level[i]=0;
		royal_memo.multiplier_value[i]=100;
	}

	//Check if we have already initialise eeproom by read first byte that should be equal to 147 (this is arbitrary but a reset atmega should contain only 0xFF value)
	eeprom_busy_wait();
	if(eeprom_read_byte((uint8_t*)0)!=147)
	{
		eeprom_busy_wait();
		eeprom_write_byte((uint8_t*)0,147);
		for(i=0;i!=NB_MAX_MODEL_MEMORY;i++)
		{
			eeprom_busy_wait();
			eeprom_write_block(&royal_memo,(uint8_t*)1+(sizeof(royal_memo)*i),sizeof(royal_memo));
		}
	}
	eeprom_busy_wait();
	eeprom_read_block(&royal_memo,(uint8_t*)1+(sizeof(royal_memo)*i),sizeof(royal_memo));
}


void init_royal(unsigned char standard_boot)
{

  reset_telemetry();
  unsigned char evo_return_after_first_nego;
  evo_return_after_first_nego=init_evo_negotiation(standard_boot);

  SET_PORT_LOW(A,7);

  _delay_ms(10);
  end_evo_transaction(evo_return_after_first_nego);

  SET_PORT_HIGH(A,7);
  //We use OCR0B Register as a semaphore that tell us if we have a new frame ready to be handle
  serial0_reset_ct();
  OCR0=0;
  ooTIMER0_OVERFLOW_ON;
  ooTIMER0_SCALE_8;





}
//Reset all the telemetry value to default
//used on init and on display context switch
void reset_telemetry(void)
{
	for(int i=0;i<NB_CHANEL_TELEMETRY;i++)
	{
	   royal_tele[i].unite=0;
	   royal_tele[i].valeur=0;
	   royal_tele[i].high_valeur=-10000;
	   royal_tele[i].low_valeur=+10000;
	}
}
//This function decode royal evo frame by reading the linear  UART buffer
void decode_evo_data(void)
{
  unsigned char i;
  //unsigned char buffer_ct=0;

  signed int revert_value;

  per_frame_error=0;
  if (emulation_mode)
      {nbvoies=5+emulation_mode;} // 5 + nbre choisi par les cavaliers 
  else
      {nbvoies=evo_uart_direct_buffer_read(0);} //1ere donn√àe = nbre de voies

  if (nbvoies>12) {nbvoies=12;} //PPM12 au maximum
  if (nbvoies<6) {nbvoies=6;}; //PPM6 au minimum
  //et c'est parti pour la conversion des donnees
  nbvoies=8;


  for (i=0;i<nbvoies;i++)
  {
	  revert_value = MPX_voie[i];
      /*
	  buffer_ct++;
      MPX_voie[i]= (evo_uart_direct_buffer_read(buffer_ct));
      buffer_ct++;
      MPX_voie[i]+=((evo_uart_direct_buffer_read(buffer_ct))<<8);
		*/

      MPX_voie[i]=((evo_uart_direct_buffer_read(2*i+2))<<8);
      MPX_voie[i]+= (evo_uart_direct_buffer_read(2*i+1));
      if (MPX_voie[i]>=0x8000) {MPX_voie[i]=(signed int) (MPX_voie[i]-0x10000);}

      //if (MPX_voie[i]>=0x8000) {MPX_voie[i]=(signed int) (MPX_voie[i]-0x10000);}

      //CHECK IF VALUE ARE CORRECT (COULD HAVE SOME LINE TROUBLE)

      if(MPX_voie[i]>ROYAL_MAX_CHANEL_VALUE || MPX_voie[i]<-ROYAL_MAX_CHANEL_VALUE )
            {
            	  MPX_voie[i]=revert_value;
            	  per_cycle_error++;
            	  per_frame_error++;
            }
      //END_DEBUG

      //Translate for PPM output


#if F_CPU == 11059200
      set_ppm1_chanel(i,MPX_voie[i]-(MPX_voie[i]>>1));
#elif F_CPU == 7372800
      set_ppm1_chanel(MPX_voie[i]*16/44);
#endif


              
			
  }
  //We restart the timer0 to trigger the next input from RoyalEvo
  //We cycle on 100 Frame for error presence evaluation
  frame_counter++;
  if( frame_counter==100)
  {
	  frame_counter=0;
	  per_cycle_error=0;
  }


}
//*********************************************************************************************
//* NEGOTIATION ROYAL EVO (AT STARTUP DIALOG TO FIND OUT WHAT KIND OF MODULE IS CONNECTED)
//*********************************************************************************************


unsigned char init_evo_negotiation(unsigned char standard_boot)
{

	//evo_uart_close();
	evo_uart_init(19200);
  unsigned char c=0;

  unsigned int escape=0;
  do
	{
	  // DIRTY  EXIT FOR IN CASE OF BROWN_OUT OR  WATCH_DOG RESET
	  // I HAVE TO SETUP A TIMMING UPON WICH I PRESUME THAT NO NEGO IS NEEDED BY ROYAL EVO I START WITH A 500ms
	  if(!standard_boot)
	  {
		  _delay_ms(1);
		  escape++;
		  if(escape>500)
			  return 'g';
	  }
     if(evo_uart_NewData())
     {


  		c =  evo_uart_readchar(); //on r√àcup√ãre la derni√ãre donn√àes re√Åue
  		if (c=='v') // on r√àpond ‚Ä° la radio qui veut savoir le type de module utilis√à
  		{

			//LED_PORT |= (1<<LED_NUM);// led allum√àe
			_delay_ms(12);
			//LED_PORT &= ~(1<<LED_NUM);// led √àteinte


			evo_uart_writestring("M-LINK   V3200"); // module M-LINK 2.4 GHz V3027 V3200

			//uart_puts("40  HFMS2V268 Jul 11 200812:52:44"); //module 40 MHz
			//evo_uart_writestring("35ABHFMS2V268 Jul 11 200812:52:00"); //module 35 MHz
			//	evo_start_mode=1;

			evo_uart_writechar(0x0D);
			evo_uart_writechar(0x0A);

			//on remet le buffer au d√àbut (en principe, ‚Ä° ce moment, il n'y a qu'un octet)

		}
	      
		// LA RADIO DEMANDE LA FREQUENCE UTILISEE AU MODULE
		if ((c=='?') | (c=='+') | (c=='-')) // on r√àpond ‚Ä° la radio qui veut connaitre la fr√àquence utilis√à
		{

			_delay_ms(12);

			evo_uart_writestring("35AB2G402400"); // module 35 MHz  35AB2G402400

			//uart_puts("40  051406750"); // module 40 MHz
			//avec un module MLINK, la radio ne demande pas la fr√àquence
			evo_uart_writechar(0x01); // ??
		

			_delay_ms(12);
			//evo_uart_writechar(0x03); // ?? //scanner 35 MHz r√àpond que la fr√àquence est libre
			//evo_uart_writechar(0x0F); // ??
			//evo_uart_writechar('0'); // ??
			evo_uart_writechar(0x40); // ?? // pas de scanner 40 MHz, il faut confirmer qu'on veut utiliser cette fr√àquence
			evo_uart_writechar(0x0A); // ??
			evo_uart_writechar(0x33); // ??
		
			evo_uart_writechar(0x0D); // fin de la transmission
			evo_uart_writechar(0x0A);

			//LED_PORT |= (1<<LED_NUM);// led allum√àe
			_delay_ms(12);
			//LED_PORT &= ~(1<<LED_NUM);// led √àteinte
		
			//on remet le buffer au d√àbut (en principe, ‚Ä° ce moment, il n'y a qu'un octet)
			
		}

       }

	} while ((c!='b') & (c!='a') & (c!='r'));
	
	return c;
}		

	
void end_evo_transaction(unsigned char c)
{
    unsigned int i;
    //evo_uart_close();
    evo_uart_change_rate(115200);
	if (c=='b') // demande de binding (uniquement module 2.4GHz)
	{
                
		// pour patienter un peu que tout se mette en place !
		_delay_ms(8); // OK pour le module 2.4 GHz
		for (i=0;i<500;i++)  // binding en cours (fake)
		{	
			send_bind();  
			_delay_ms(28);
		}
		for (i=0;i<3;i++)
		{
			_delay_ms(250); // delai de 750 ms
		}
		for (i=0;i<11;i++)  // fin du binding
		{	
			send_nobind();  
			_delay_ms(20); 
		}
	} 
	if (c=='r') // test de port√àe (uniquement module 2.4GHz)
	{
		// pour patienter un peu que tout se mette en place !
		_delay_ms(8); // OK pour le module 2.4 GHz
		for (i=0;i<500;i++)  // test en cours (fake)
		{	
			send_range();  
			_delay_ms(28);
		}
		for (i=0;i<3;i++)
		{
			_delay_ms(250); // delai de 750 ms
		}
		for (i=0;i<11;i++)  // fin du test
		{	
			send_nobind();  
			_delay_ms(20);
		}
	} 
	if (c=='a') // c'est parti !
	{
		// pour patienter un peu que tout se mette en place !
		//_delay_ms(200); // OK avec les modules 35 et 40 MHz

		_delay_ms(2); // OK pour le module 2.4 GHz
		//for (i=0;i<3;i++)  // pas de binding
		//{	
		//	send_nobind();  
		//	_delay_ms(14); // mode FastResponse ?
		//}
	}	
	
}

void send_commonbind(void) //inutile pour un module 35 ou 40 MHz
{
	// 4 octets qui sont pr√àsents, binding en cours ou pas
	evo_uart_writechar(0xFF); // ??
	evo_uart_writechar(0x00); // ??
	evo_uart_writechar(0x00); // ??
	evo_uart_writechar(0x00); // ??
}
void send_bind(void) //inutile pour un module 35 ou 40 MHz
{
	evo_uart_writechar(0x20); // ?? // 0x00 si pas de binding, 0x20 si binding en cours
	send_commonbind();
}

void send_nobind(void) //inutile pour un module 35 ou 40 MHz
{
	evo_uart_writechar(0x00); // ?? // 0x00 si pas de binding, 0x20 si binding en cours
	send_commonbind();
}

void send_range(void) //inutile pour un module 35 ou 40 MHz
{
	evo_uart_writechar(0x40);
	send_commonbind();
}

//*********************************************************************************************
//* TELEMETRY
//*********************************************************************************************
// #1+2#
void send_telemetry(unsigned char position, unsigned char unite,signed int valeur, unsigned char alarme)
{

        //if(position!=2) 
         // return;
	valeur=valeur*2; // on multiplie par 20 (format Multiplex)
	//valeur=valeur*10; //inutile depuis le firmware 3.41
	if (alarme) valeur++; // on ajoute 1 si on veut l'alarme
	 
	//1er octet (alarme de port√àe)
	if (evo_rssi<=30) evo_uart_writechar(0x40); else evo_uart_writechar(0x00);
	//2e octet
	evo_uart_writechar(0x01);// pas selon stoeckli
	//3e octet (position ‡ l'Èàcran + unitÈeà de mesure)
	evo_uart_writechar( (position<<4) + unite );
	//4e octet (poids faible de la valeur)
	evo_uart_writechar( (char)valeur );
	//5e octet (poids fort de la valeur)
	evo_uart_writechar( (char)(valeur>>8) );
	// on dirait qu'il faut finir par OO selon stoeckli
	evo_uart_writechar(0x00);
}


void send_evo_telemetry()
{


  if(royal_tele[ evo_tele_ct].unite==0)
    return;
  signed int valeur=0;
if(evo_cursor_timestamp!=255)
	evo_cursor_timestamp++;
  switch(evo_display_mode)
  {
  	  case EVO_DISPLAY_NORMAL:
  	  	  valeur=royal_tele[evo_tele_ct].valeur;
  	  break;

  	  case EVO_DISPLAY_LOW:
  		valeur=royal_tele[evo_tele_ct].low_valeur;
  	  break;

  	  case EVO_DISPLAY_HIGH:
  		valeur=royal_tele[evo_tele_ct].high_valeur;
  	  break;
  }
  valeur=valeur*2; // on multiplie par 20 (format Multiplex)
  //valeur=valeur*10; //inutile depuis le firmware 3.41
  if (royal_tele[evo_tele_ct].alarme) valeur++; // on ajoute 1 si on veut l'alarme

  //Byte 1
  if (evo_rssi<=evo_alarm && evo_alarm!=0)
	  evo_uart_writechar(0x40);
  else
	  evo_uart_writechar(0x00);

  //Byte 2
  evo_uart_writechar(0x01);// pas selon stoeckli

  //Byte 3
  evo_uart_writechar( (evo_tele_ct<<4) + royal_tele[evo_tele_ct].unite );


  if(evo_cursor_blink==evo_tele_ct && evo_cursor_timestamp!=255)
  {
	  if( (evo_cursor_blink_state=!evo_cursor_blink_state) )
		  valeur=valeur;
	  else
		  if(valeur==0)
		  valeur=-(valeur+2);
		  else
		  valeur=-valeur;
  }

  //4e octet (poids faible de la valeur)
  evo_uart_writechar( (char)valeur );
  //5e octet (poids fort de la valeur)
  evo_uart_writechar( (char)(valeur>>8) );
  // on dirait qu'il faut finir par OO selon stoeckli
  evo_uart_writechar(0x00);
  evo_tele_ct++;

  //Avoid overflow
  if(evo_tele_ct== NB_CHANEL_TELEMETRY)
	  evo_tele_ct=0;
 //we stop if we reach the last assigned telemetry chanel
  if(royal_tele[ evo_tele_ct].unite==0 )
	  evo_tele_ct=0;
}
/**
 * Set the level of RSSI alarm of royal Evo if set at 0 there will never be Royal Evo Alarm
 */
void set_evo_rssi_alarm_level(unsigned char evo_alarm_param)
{
	if(evo_alarm_param==0)
		evo_alarm_param=100;
	evo_alarm=evo_alarm_param;

}

/**
 * Set the current evo display mode (normale
 */
void set_evo_display_mode(unsigned char displaymode)
{
	evo_display_mode=displaymode;
}

unsigned char get_evo_display_mode(void)
{
	return evo_display_mode;
}
unsigned char evo_cursor_active(void)
{
	return evo_cursor_timestamp!=255;
}

void evo_cursor_down(void)
{
	evo_cursor_blink++;
	if(evo_cursor_blink== NB_CHANEL_TELEMETRY)
		evo_cursor_blink=0;
	if(royal_tele[ evo_cursor_blink].unite==0)
		evo_cursor_blink=0;
	evo_cursor_timestamp=0;
}
void evo_cursor_up(void)
{
	if(evo_cursor_blink==0)
		evo_cursor_blink=0;
	else
		evo_cursor_blink--;
	evo_cursor_timestamp=0;
}
unsigned char evo_get_cursor_pos(void)
{
	return evo_cursor_blink;
}
void evo_reset_input_selector(void)
{
	unsigned char i;
	MPX_voie_current_selector=0xFF;
	for(i=0;i!=NB_CHANEL_EVO;i++)
		MPX_voie_selector[i]=MPX_voie[i];
}
signed int evo_input_selector_value(void)
{
	unsigned char i;
	//We do not know right now wich will be our channel selector
	if(MPX_voie_current_selector==0xFF)
	{
		for(i=0;i!=NB_CHANEL_EVO;i++)
			if(MPX_voie_selector[i]>(MPX_voie[i]+CHANEL_SELECTOR_THRESOLD) || MPX_voie_selector[i]<(MPX_voie[i]-CHANEL_SELECTOR_THRESOLD))
			MPX_voie_current_selector=i;


		return 0;
	}
	else
		return MPX_voie[MPX_voie_current_selector]-MPX_voie_selector[MPX_voie_current_selector];
}



