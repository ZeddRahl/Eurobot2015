/*
* Sick
* Compiler : Microchip xC16
* �C : 33FJ64MC804
* Mai 2015
*    ____________      _           _
*   |___  /| ___ \    | |         | |
*      / / | |_/ /___ | |__   ___ | |_
*     / /  |    // _ \| '_ \ / _ \| __|
*    / /   | |\ \ (_) | |_) | (_) | |_
*   /_/    |_| \_\___/|____/ \___/'\__|
*			      7robot.fr
*/

/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__dsPIC33E__)
    	#include <p33Exxxx.h>
    #elif defined(__dsPIC33F__)
    	#include <p33Fxxxx.h>
    #endif
#endif

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdio.h>
#include <stdbool.h>       /* Includes true/false definition                  */

#include "main.h"
#include <timer.h>


/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

volatile int channel = 0;		// canal de conversion actuel
volatile uint8_t i_moy_sick = 0;
volatile uint16_t Value_Sick[NUMBER_OF_SICK][NUMBER_OF_POINTS_MOY_SICK];	// r�cup�rere la valeur de l'ADC du sick, puis fait une moyenne tournante
volatile uint32_t Sum_Value_Sick[NUMBER_OF_SICK];
volatile uint16_t Old_Sector[NUMBER_OF_SICK] = {0};		// full of 0	// tout ou rien sur les seuils

volatile uint16_t Threshold[NUMBER_OF_SICK] = {DEFAULT_THRESHOLD, DEFAULT_THRESHOLD, DEFAULT_THRESHOLD, DEFAULT_THRESHOLD};

volatile uint16_t V_Bat = 0;
volatile uint16_t V_Bat_Tab[NUMBER_MEAS_VBAT];

volatile uint8_t debug_sick_on = 0;
volatile uint8_t Ative_Motion_Free_Sicks = 1;
volatile uint8_t Motion_Free_Activ_Each = 0x0F;
/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

void InitSick_VBat()
{
    uint8_t i, j;
    
    // si utilisation en r�-init : on tue l'IT d'abord...
    CloseTimer5();
    _AD1IF = 0;        //Clear the interrupt flag

    // reset des systemes de moyenne tournante
	// chargement de valeurs grosses, histoire que �a d�clenche pas lors de l'init
    for (j = 0; j < NUMBER_OF_SICK; j++ ) {
        Threshold[j] = DEFAULT_THRESHOLD;
        Sum_Value_Sick[j] = 0;
        Old_Sector[j] = 1;
        for (i = 0; i < NUMBER_OF_POINTS_MOY_SICK; i++){
            Value_Sick[j][i] = 512;
            Sum_Value_Sick[j] += 512;
        }
    }
    channel = 0;
    i_moy_sick = 0;

    for (j = 0; j < NUMBER_MEAS_VBAT; j++) {
        V_Bat_Tab[j] = 40000;
    }

    //Configuration du convertisseur Analog to Digital (ADC) du dspic33f
    //Cf page 286 dspic33f Data Sheet

    //AD1CON1 Configuration
    AD1CON1bits.ADON = 0;    //Eteindre A/D converter pour la configuration
    AD1CON1bits.FORM = 0;    //Configure le format de la sortie de l'ADC ( 3=signed float, 2=unsigned float, 1=signed integer, 0=unsigned integer
    AD1CON1bits.SSRC = 4;    //Config de l'échantillonnage : Timer5
    AD1CON1bits.SIMSAM = 0;  //Simultaneously Sample CH0
    AD1CON1bits.ASAM = 1;    //Début d'échantillonnage (1=tout de suite  0=dès que AD1CON1bits.SAMP est activé)
    AD1CON1bits.AD12B = 0;   //Choix du type de converter (10 ou 12 bits) 0 = 10 bits , 1 = 12bits

    //AD1CON2 Configuration
    AD1CON2bits.ALTS = 0;     //Always sampling on channel A
    AD1CON2bits.CHPS = 0;    //Select CH0

    //AD1CON3 Configuration
    AD1CON3bits.ADRC = 1;        //Choix du type de clock interne (=1) ou externe (=0)


    //Choix des références de tensions
                                                        // Choix du (+) de la mesure pour le channel CH0, commençant par le sick 1
    AD1CHS0bits.CH0SA = AN_CH_SICK_ARRIERE_DROIT;
    AD1CHS0bits.CH0NA = 0;	// Choix du (-) de la mesure pour le channel CH0 (0 = Masse interne pic)

    //Configuration des pins analogiques
    //AD1PCFGL = 0xFFFF;   //Met tous les ports AN en Digital Input

    /* COM A ENLEVER SUR DSPIC AVEC 8 PINS ANALOGIQUES */
    // sicks :
    AD1PCFGLbits.PCFG4 = 0;
    AD1PCFGLbits.PCFG5 = 0;
    AD1PCFGLbits.PCFG6 = 0;
    AD1PCFGLbits.PCFG7 = 0;
    // Vbat
    AD1PCFGLbits.PCFG0 = 0;




    //Configuration du Timer 5, pour l'ADC
    // OpenTimer5(T5_ON & T5_GATE_OFF & T5_PS_1_256 & T5_SOURCE_INT, 15625); from 2014
    OpenTimer5(T5_ON & T5_GATE_OFF & T5_PS_1_8 & T5_SOURCE_INT, 3125 ); // for 2015
    // FCY = 40Meg
    // prescaller à 8 et comparaison à 3125 => 40M / (8*3125) = 1600 => 1600 départ de comparaison / sec
    // avec 4 canaux, ça fait 400 detections / sec 
    // attention : (moyennage sur 16)


    //Configuration des interuption
    IFS0bits.AD1IF = 0;      //Mise à 0 du flag d'interrupt de ADC1
    IEC0bits.AD1IE = 1;      //Enable les interruptions d'ADC1
    IPC3bits.AD1IP = 2;      //Et les priorités (ici prio = 2)
    AD1CON1bits.SAMP = 0;
    AD1CON1bits.ADON = 1;    // Turn on the A/D converter
}

void OnSickThreshold(unsigned char id, unsigned int threshold)
{
    int i=0;
    if(id == 255) {
        for (i=0;i<NUMBER_OF_SICK;i++){
            Threshold[i] = threshold;
        }
    } else if(id >= 0 && id < NUMBER_OF_SICK) {
        Threshold[id] = threshold;
    }
}

void OnAskSick(unsigned char id){
   // SendSickState(id,Old_Sector[id]);
}

/******************************************************************************/
/* Interrupt                                                                  */
/******************************************************************************/

void __attribute__ ((interrupt, auto_psv)) _ADC1Interrupt(void)
{
    static uint16_t timer_for_VBat = TIMER_FOR_VBAT;
    static uint8_t i_V_Bat_tab = 0;
    uint8_t i;
    uint32_t val32;

    static uint16_t i_debug_sick = 0;
    uint16_t val16 = ADC1BUF0;// récupération valeur depuis ADC

    if (channel != NUMBER_OF_SICK) {
        Sum_Value_Sick[channel] -= Value_Sick[channel][i_moy_sick];     // enl�ve la valeur de X coups d'avant
        Sum_Value_Sick[channel] += val16;                               // ajoute la valeur de maintenant

        Value_Sick[channel][i_moy_sick] = val16;                        // enregistrement de la valeur lue

        val16 = (uint16_t)(Sum_Value_Sick[channel] >> 4);   // r�cup de la somme et division par 16

        if (Old_Sector[channel] == 0) {     // si on consid�re pour l'instant qu'il y a un truc "pres"
            if (val16 > (Threshold[channel] + MARGIN_SICK)) {  // si la valeur repasse au dessus de seuil + marge
                Old_Sector[channel] = 1;        // on repasse en zone "s�re"
                ReleaseSick(channel);			// on previent la PI
            }
        } else {    // if old = 1   // si, pour l'instant, il n'y a pas de truc "pres"
            if ( (val16 < (Threshold[channel] - MARGIN_SICK))  && (val16 > SICK_LIMIT_MIN)  ) {   // si on vient de detecter un truc
               Old_Sector[channel] = 0;     // on passe en zone "pas s�re"
               if (Motion_Free_Activ_Each & (0x01<<channel)) {      // v�rif sick par sick
                    motion_free();                  // et on gueule aupr�s de l'asserv
               }
                DetectSick(channel);				// on previent la PI
            }
        }

        if (debug_sick_on) {
            if (i_debug_sick == 3000) {
                printf ("Sick 1 : %ld\nSick 2 : %ld\nSick 3 : %ld\nSick 4 : %ld;", (Sum_Value_Sick[0]), (Sum_Value_Sick[1]), (Sum_Value_Sick[2]), (Sum_Value_Sick[3]) );
                i_debug_sick = 0;
            } else {
                i_debug_sick ++;
            }
        }
    } else {    // pour VBAT
        // de 0 � 1023 = 0 => 3.3V
        // avec pont div de 11
        // => 35.44.... mV de baterie par point
        // = * 9075  puis div par 512  soit >> 8

        V_Bat_Tab[i_V_Bat_tab] = (uint16_t)(((9075 * ((uint32_t)(val16))) >> 8));
        
        i_V_Bat_tab ++;
        if (i_V_Bat_tab == NUMBER_MEAS_VBAT) {
            i_V_Bat_tab = 0;
        }
        
        val32 = 0;
        for (i = 0; i < NUMBER_MEAS_VBAT; i++) {
            val32 += V_Bat_Tab[i];
        }
        val32 = val32 >> 3;
        V_Bat = (uint16_t)(val32);
    }

    // Select next sensor 
    switch(channel)     
    {
        case 0:
            _CH0SA = AN_CH_SICK_ARRIERE_GAUCHE;		// sick 2
            channel = 1;
            break;
        case 1:
            _CH0SA = AN_CH_SICK_AVANT_DROIT;		// sick 3
            channel = 2;
            break;
        case 2:
            _CH0SA = AN_CH_SICK_AVANT_GAUCHE;		// sick 4
            channel = 3;
            break;
        case 3:
            timer_for_VBat --;
            if (timer_for_VBat) {
                _CH0SA = AN_CH_SICK_ARRIERE_DROIT;		// sick 1
                channel = 0;
            } else {
               timer_for_VBat = TIMER_FOR_VBAT;
               _CH0SA = AN_CH_V_BAT;                            // V BAT
               channel = 4;
            }
            i_moy_sick++;
            if (i_moy_sick == NUMBER_OF_POINTS_MOY_SICK) {
                i_moy_sick = 0;
            }
            break;
        case  4:
            _CH0SA = AN_CH_SICK_ARRIERE_DROIT;                  // sick 1
            channel = 0;
            break;
    }
    _AD1IF = 0;        //Clear the interrupt flag
 }

uint16_t Get_Sick(uint8_t Sick_Voulu)
{
    if (Sick_Voulu < NUMBER_OF_SICK) {
        return (uint16_t)(Sum_Value_Sick[Sick_Voulu] >> 4);
    } else {
        return 0;
    }
}

// return 1 si rien devant
// return 0 si detection
uint16_t Get_Sick_Sector (uint8_t Sick_Voulu)
{
    if (Sick_Voulu < NUMBER_OF_SICK) {
        return Old_Sector[Sick_Voulu];
    } else {
        return 0;
    }
}

void Start_Stop_Debug_Sick(void)
{
    if (debug_sick_on) {
        debug_sick_on = 0;
    } else {
        debug_sick_on = 1;
    }
}

void Choose_Enabled_Sicks(char Sicks_En)
{
    Motion_Free_Activ_Each = Sicks_En;
}

   

