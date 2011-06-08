/***********************************************************************************************************/
// This Section handle all the transaction IN & OUT Between Arduino and Royal Evo                         
// Almost all the code is derivated or copied from the excellent work of YannickF Project MPX2PPM        
// http://www.yannickf.net/spip/spip.php?article106
/***********************************************************************************************************/
#include <util/delay.h>

#include "write_ppm.h"
#include "serial.h"
#include "royal_evo.h"

signed int MPX_voie[16]; //Multiplex Data Chanel
char nbvoies=0; 
unsigned char emulation_mode=1;
unsigned int evo_rssi=100;

royal_tememetry_struct  royal_tele[14];
void init_royal()
{
  for(int i=0;i<12;i++)
   royal_tele[i].unite=0;
}
//This function decode royal evo frame by reading the linear lowlevel UART buffer
void decode_evo_data()
{
  unsigned char i;
  unsigned char buffer_ct=0;
  if (emulation_mode)
      {nbvoies=5+emulation_mode;} // 5 + nbre choisi par les cavaliers 
  else
      {nbvoies=Read_Low_Level_Serial_Buffer(0);} //1ere donnÈe = nbre de voies

  if (nbvoies>12) {nbvoies=12;} //PPM12 au maximum
  if (nbvoies<6) {nbvoies=6;}; //PPM6 au minimum
  //et c'est parti pour la conversion des donnees
  nbvoies=8;

  
  for (i=0;i<nbvoies;i++)
  {
             
      buffer_ct++;
      MPX_voie[i]= (Read_Low_Level_Serial_Buffer(buffer_ct));
      buffer_ct++;
      MPX_voie[i]+=((Read_Low_Level_Serial_Buffer(buffer_ct))<<8);
      
      if (MPX_voie[i]>=0x8000) {MPX_voie[i]=(signed int) (MPX_voie[i]-0x10000);}

      set_ppm1_chanel(i,MPX_voie[i]-(MPX_voie[i]>>2)+(MPX_voie[i]>>7));
              
			
  }
		
	
		
               
		
		
}
//*********************************************************************************************
//* NEGOTIATION ROYAL EVO (AT STARTUP DIALOG TO FIND OUT WHAT KIND OF MODULE IS CONNECTED)
//*********************************************************************************************
unsigned char init_evo_negotiation()
{
  unsigned char c;
  unsigned int escape=0;
  do
	{
              // DIRTY SPEED EXIT FOR DEV ?
              escape++;
               if(escape==65500)
                 return 'g';
                
             if(serial_data_waiting())
             {
               
  		c =  serial_read_last(); //on rÈcupËre la derniËre donnÈes reÁue
  		if (c=='v') // on rÈpond ‡ la radio qui veut savoir le type de module utilisÈ
  		{
			//LED_PORT |= (1<<LED_NUM);// led allumÈe
			_delay_ms(12);
			//LED_PORT &= ~(1<<LED_NUM);// led Èteinte
			
			if (emulation_mode)
			{uart_puts("M-LINK   V3027");} // module M-LINK 2.4 GHz
			else
			{
			//uart_puts("40  HFMS2V268 Jul 11 200812:52:44"); //module 40 MHz
			uart_puts("35ABHFMS2V268 Jul 11 200812:52:00"); //module 35 MHz
			}
			uart_putc(0x0D);
			uart_putc(0x0A);
		
			//on remet le buffer au dÈbut (en principe, ‡ ce moment, il n'y a qu'un octet)

		}
	      
		// LA RADIO DEMANDE LA FREQUENCE UTILISEE AU MODULE
		if ((c=='?') | (c=='+') | (c=='-')) // on rÈpond ‡ la radio qui veut connaitre la frÈquence utilisÈ
		{
			_delay_ms(12);
			uart_puts("35AB2G402400"); // module 35 MHz
			//uart_puts("40  051406750"); // module 40 MHz
			//avec un module MLINK, la radio ne demande pas la frÈquence
			uart_putc(0x01); // ??
		

			_delay_ms(12);
			//uart_putc(0x03); // ?? //scanner 35 MHz rÈpond que la frÈquence est libre
			//uart_putc(0x0F); // ??
			//uart_putc('0'); // ??
			uart_putc(0x40); // ?? // pas de scanner 40 MHz, il faut confirmer qu'on veut utiliser cette frÈquence
			uart_putc(0x0A); // ??	
			uart_putc(0x33); // ??
		
			uart_putc(0x0D); // fin de la transmission
			uart_putc(0x0A);

			//LED_PORT |= (1<<LED_NUM);// led allumÈe
			_delay_ms(12);
			//LED_PORT &= ~(1<<LED_NUM);// led Èteinte
		
			//on remet le buffer au dÈbut (en principe, ‡ ce moment, il n'y a qu'un octet)
			
		}
           
            }	

	} while ((c!='b') & (c!='a') & (c!='r'));
	
	return c;
}		

	
void end_evo_transaction(unsigned char c)
{
        unsigned int i;
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
	if (c=='r') // test de portÈe (uniquement module 2.4GHz)
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
	// 4 octets qui sont prÈsents, binding en cours ou pas
	uart_putc(0xFF); // ??	
	uart_putc(0x00); // ??	
	uart_putc(0x00); // ??	
	uart_putc(0x00); // ??
}
void send_bind(void) //inutile pour un module 35 ou 40 MHz
{
	uart_putc(0x20); // ?? // 0x00 si pas de binding, 0x20 si binding en cours
	send_commonbind();
}

void send_nobind(void) //inutile pour un module 35 ou 40 MHz
{
	uart_putc(0x00); // ?? // 0x00 si pas de binding, 0x20 si binding en cours
	send_commonbind();
}

void send_range(void) //inutile pour un module 35 ou 40 MHz
{
	uart_putc(0x40); 
	send_commonbind();
}

//*********************************************************************************************
//* TELEMETRY
//*********************************************************************************************
void send_telemetry(unsigned char position, unsigned char unite,signed int valeur, unsigned char alarme)
{
  
        //if(position!=2) 
         // return;
	valeur=valeur*2; // on multiplie par 20 (format Multiplex)
	//valeur=valeur*10; //inutile depuis le firmware 3.41
	if (alarme) valeur++; // on ajoute 1 si on veut l'alarme
	 
	//1er octet (alarme de portÈe)
	if (evo_rssi<=30) uart_putc(0x40); else uart_putc(0x00); 
	//2e octet
	uart_putc(0x01);// pas selon stoeckli
	//3e octet (position ‡ l'Ècran + unitÈ de mesure)
	uart_putc( (position<<4) + unite );
	//4e octet (poids faible de la valeur)
	uart_putc( (char)valeur );
	//5e octet (poids fort de la valeur)
	uart_putc( (char)(valeur>>8) );
	// on dirait qu'il faut finir par OO selon stoeckli
	uart_putc(0x00);
}

unsigned char evo_tele_ct=0;
void send_evo_telemetry()
{

  if(royal_tele[ evo_tele_ct].unite==0)
    return;
  signed int valeur = royal_tele[evo_tele_ct].valeur;
  valeur=valeur*2; // on multiplie par 20 (format Multiplex)
  //valeur=valeur*10; //inutile depuis le firmware 3.41
  if (royal_tele[evo_tele_ct].alarme) valeur++; // on ajoute 1 si on veut l'alarme

  //1er octet (alarme de portÈe)
  if (evo_rssi<=30) uart_putc(0x40); 
  else uart_putc(0x00); 
  //2e octet
  uart_putc(0x01);// pas selon stoeckli
  //3e octet (position ‡ l'Ècran + unitÈ de mesure)
  uart_putc( (evo_tele_ct<<4) + royal_tele[evo_tele_ct].unite );
  //4e octet (poids faible de la valeur)
  uart_putc( (char)valeur );
  //5e octet (poids fort de la valeur)
  uart_putc( (char)(valeur>>8) );
  // on dirait qu'il faut finir par OO selon stoeckli
  uart_putc(0x00);
  evo_tele_ct++;
  if(royal_tele[ evo_tele_ct].unite==0)
   evo_tele_ct=0;
}

